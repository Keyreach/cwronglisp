#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "astlist.h"

#define SWITCH_OPERATOR(x) if(strcmp(operator, x) == 0){
#define CASE_OPERATOR(x) } else if(strcmp(operator, x) == 0) {
#define CASE_DEFAULT } else {
#define CASE_END }

ASTList* GlobalInterpreterScope = NULL;

char* Substring(char* text, size_t start, size_t end){
    size_t len = end - start;
    char* s = (char*)malloc(len + 1);
    memcpy(s, text + start, len);
    s[len] = '\0';
    return s;
}

ASTList* Tokenize(char *code){
    ASTList *tokens = NULL;
    ASTItem item;
    size_t tokenStart = -1;
    char c, quoted = 0;
    size_t i, n = strlen(code);
    for(i = 0; i < n; i++){
        c = code[i];
        if(quoted){
            if(c == '"'){
                item.str = Substring(code, tokenStart, i);
                astListAppend(&tokens, item, ASTNODE_STR);
                tokenStart = -1;
                quoted = 0;
            }
        } else {
            switch(c){
                case '(':
                case ')':
                    if(tokenStart != -1){
                        item.str = Substring(code, tokenStart, i);
                        astListAppend(&tokens, item, ASTNODE_STR);
                        tokenStart = -1;
                    }
                    item.str = (c == '(' ? "(" : ")");
                    astListAppend(&tokens, item, ASTNODE_STR);
                    break;
                case '{':
                case '}':
                    if(tokenStart != -1){
                        item.str = Substring(code, tokenStart, i);
                        astListAppend(&tokens, item, ASTNODE_STR);
                        tokenStart = -1;
                    }
                    item.str = c == '{' ? "(" : ")";
                    astListAppend(&tokens, item, ASTNODE_STR);
                    if(c == '{'){
                        item.str = "quote";
                        astListAppend(&tokens, item, ASTNODE_STR);
                    }
                    break;
                case '"':
                    if(tokenStart != -1){
                        item.str = Substring(code, tokenStart, i);
                        astListAppend(&tokens, item, ASTNODE_STR);
                        tokenStart = -1;
                    }
                    tokenStart = i + 1;
                    quoted = 1;
                    break;
                case ' ':
                case '\n':
                case '\t':
                    if(tokenStart != -1){
                        item.str = Substring(code, tokenStart, i);
                        astListAppend(&tokens, item, ASTNODE_STR);
                        tokenStart = -1;
                    }
                    break;
                default:
                    if(tokenStart == -1){
                        tokenStart = i;
                    }
            }
        }
    }
    if(tokenStart != -1){
        item.str = Substring(code, tokenStart, i);
        astListAppend(&tokens, item, ASTNODE_STR);
        tokenStart = -1;
    }
    return tokens;
}

ASTList* Parser(ASTList* tokens){
    ASTList *tree = NULL;
    ASTList *branch = NULL;
    ASTItem item;
    char *token;
    size_t brackets = 0;
    size_t i, n = astListLen(&tokens);
    for(i = 0; i < n; i++){
        token = astListGet(tokens, i)->value.str;
        if(strcmp(token, "(") == 0){
            if(brackets > 0){
                item.str = token;
                astListAppend(&branch, item, ASTNODE_STR);
            }
            brackets++;
        } else if(strcmp(token, ")") == 0) {
            brackets--;
            if(brackets > 0){
                item.str = token;
                astListAppend(&branch, item, ASTNODE_STR);
            } else {
                item.list = Parser(branch);
                astListAppend(&tree, item, ASTNODE_LIST);
                astListEmpty(&branch);
            }
        } else {
            item.str = token;
            if(brackets > 0){
                astListAppend(&branch, item, ASTNODE_STR);
            } else {
                astListAppend(&tree, item, ASTNODE_STR);
            }
        }
    }
    return tree;
}

ASTList* ExecList(ASTList *ast){
    char* operator;
    ASTList *a, *b, *tmp, *cond, *retval;
    ASTItem item;
    if(ast->type == ASTNODE_STR){
        //printf("atom: %s\n", ast->value.str);
        return ast;
    }
    //printf("s-expr:\n");
    operator = ExecList(ast->value.list)->value.str;
    //printf("operator: %s\n", operator);

    SWITCH_OPERATOR("int")
        a = ExecList(astListGet(ast->value.list, 1));
        if(a == NULL){
            puts("int: nullptr exception");
            return NULL;
        } else if(a->type != ASTNODE_STR){
            puts("int: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = strtol(a->value.str, NULL, 10);
        return retval;

    CASE_OPERATOR("do")
        retval = NULL;
        a = ast->value.list->next;
        while(a != NULL){
            if(retval != NULL) free(retval);
            retval = ExecList(a);
            a = a->next;
        }
        return retval;

    CASE_OPERATOR("quote")
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_LIST;
        retval->value.list = astListSlice(ast->value.list, 1, 0);
        return retval;

    CASE_OPERATOR("set")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if((a == NULL) || (b == NULL)){
            puts("set: nullptr exception");
            return NULL;
        } else if(a->type != ASTNODE_STR){
            puts("set: wrong operands type");
            return NULL;
        }
        retval = astListFind(GlobalInterpreterScope, a->value, ASTNODE_STR);
        if(retval == NULL){
            item.str = a->value.str;
            astListAppend(&GlobalInterpreterScope, item, ASTNODE_STR);
            astListAppend(&GlobalInterpreterScope, b->value, b->type);
            return b;
        } else {
            // add replace to astlist.c
            tmp = retval->next->next;
            free(retval->next);
            retval->next = (ASTList*)malloc(sizeof(ASTList));
            retval->next->next = tmp;
            retval->next->value = b->value;
            retval->next->type = b->type;
            return b;
        }

    CASE_OPERATOR("get")
        a = ExecList(astListGet(ast->value.list, 1));
        if(a == NULL){
            puts("get: nullptr exception");
            return NULL;
        } else if(a->type != ASTNODE_STR){
            puts("get: wrong operands type");
            return NULL;
        }
        retval = astListFind(GlobalInterpreterScope, a->value, ASTNODE_STR);
        return retval->next;

    CASE_OPERATOR("print")
        a = ExecList(astListGet(ast->value.list, 1));
        if(a == NULL){
            puts("print: nullptr exception");
            return NULL;
        } else if(a->type == ASTNODE_STR){
            printf("%s\n", a->value.str);
        } else if(a->type == ASTNODE_NUM){
            printf("%ld\n", a->value.num);
        } else if(a->type == ASTNODE_LIST){
            astListPrint(a->value.list);
            printf("\n");
        } else {
            puts("print: wrong operand type");
        }
        return NULL;

    CASE_OPERATOR("add")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != ASTNODE_NUM) || (b->type != ASTNODE_NUM)){
            puts("add: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = a->value.num + b->value.num;
        return retval;

    CASE_OPERATOR("sub")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != ASTNODE_NUM) || (b->type != ASTNODE_NUM)){
            puts("add: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = a->value.num - b->value.num;
        return retval;

    CASE_OPERATOR("mul")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != ASTNODE_NUM) || (b->type != ASTNODE_NUM)){
            puts("add: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = a->value.num * b->value.num;
        return retval;

    CASE_OPERATOR("div")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != ASTNODE_NUM) || (b->type != ASTNODE_NUM)){
            puts("add: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = a->value.num / b->value.num;
        return retval;

    CASE_OPERATOR("mod")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != ASTNODE_NUM) || (b->type != ASTNODE_NUM)){
            puts("add: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = a->value.num % b->value.num;
        return retval;

    CASE_OPERATOR("lt")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if(a == NULL){
            puts("lt: nullptr exception");
            return NULL;
        } else if((a->type != ASTNODE_NUM) || (b->type != ASTNODE_NUM)){
            puts("lt: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = a->value.num < b->value.num;
        return retval;

    CASE_OPERATOR("gt")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if(a == NULL){
            puts("gt: nullptr exception");
            return NULL;
        } else if((a->type != ASTNODE_NUM) || (b->type != ASTNODE_NUM)){
            puts("gt: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = a->value.num > b->value.num;
        return retval;

    CASE_OPERATOR("eq")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if(a == NULL){
            puts("eq: nullptr exception");
            return NULL;
        } else if((a->type != ASTNODE_NUM) || (b->type != ASTNODE_NUM)){
            puts("eq: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = a->value.num == b->value.num;
        return retval;

    CASE_OPERATOR("ne")
        a = ExecList(astListGet(ast->value.list, 1));
        b = ExecList(astListGet(ast->value.list, 2));
        if(a == NULL){
            puts("ne: nullptr exception");
            return NULL;
        } else if((a->type != ASTNODE_NUM) || (b->type != ASTNODE_NUM)){
            puts("ne: wrong operands type");
            return NULL;
        }
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_NUM;
        retval->value.num = a->value.num != b->value.num;
        return retval;

    CASE_OPERATOR("while")
        a = astListGet(ast->value.list, 1);
        b = astListGet(ast->value.list, 2);
        cond = ExecList(a);
        while((cond != NULL) && (cond->type == ASTNODE_NUM) && (cond->value.num != 0)){
            ExecList(b);
            cond = ExecList(a);
        }

    CASE_DEFAULT
        printf("Unknown operator: %s\n", operator);

    CASE_END
    return NULL;
}

int main(){
    ASTList *head = NULL, *second = NULL, *third = NULL;
    ASTItem item;
    size_t bytes_read = 0;
    char *buffer = (char*)malloc(1024);
    NilItem.str = (char*)NULL;
    fread(buffer, 1, 1024, stdin);
    puts(buffer);
    head = Tokenize(buffer);
    //head = Tokenize("do (set x (int 1)) (while (lt (get x) (int 99)) (do (print (get x)) (set x (mul (get x) (int 2))) ))");
    puts("Lexer output:");
    astListPrint(head);
    puts("\nParser output:");
    second = Parser(head);
    astListPrint(second);
    puts("\nInterpreter output:");
    item.list = second;
    astListAppend(&third, item, ASTNODE_LIST);
    ExecList(third);
    return 0;
}
