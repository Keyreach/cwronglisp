#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "astlist.h"

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
    ASTList *a, *b, *cond, *retval;
    if(ast->type == ASTNODE_STR){
        //printf("atom: %s\n", ast->value.str);
        return ast;
    }
    //printf("s-expr:\n");
    operator = ExecList(ast->value.list)->value.str;
    //printf("operator: %s\n", operator);
    if(strcmp(operator, "int") == 0){
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
    } else if(strcmp(operator, "do") == 0) {
        retval = NULL;
        a = ast->value.list->next;
        while(a != NULL){
            if(retval != NULL) free(retval);
            retval = ExecList(a);
            a = a->next;
        }
        return retval;
    } else if(strcmp(operator, "quote") == 0) {
        retval = (ASTList*)malloc(sizeof(ASTList));
        retval->type = ASTNODE_LIST;
        retval->value.list = astListSlice(ast->value.list, 1, 0);
        return retval;
    } else if(strcmp(operator, "add") == 0) {
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
    } else if(strcmp(operator, "print") == 0) {
        a = ExecList(astListGet(ast->value.list, 1));
        if(a == NULL){
            puts("add: nullptr exception");
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
    } else if(strcmp(operator, "lt") == 0) {
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
    } else if(strcmp(operator, "gt") == 0) {
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
    } else if(strcmp(operator, "eq") == 0) {
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
    } else if(strcmp(operator, "ne") == 0) {
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
    } else if(strcmp(operator, "while") == 0) {
        a = astListGet(ast->value.list, 1);
        b = astListGet(ast->value.list, 2);
        cond = ExecList(a);
        while((cond != NULL) && (cond->type == ASTNODE_NUM) && (cond->value.num != 0)){
            ExecList(b);
            cond = ExecList(a);
        }
    } else {
        printf("Unknown operator: %s\n", operator);
    }
    return NULL;
}

int main(){
    ASTList *head = NULL, *second = NULL, *third = NULL;
    ASTItem item;
    NilItem.str = (char*)NULL;
    // head = Tokenize("do (set p (func {x} {(print (get x))})) (p \"hello world\")");
    head = Tokenize("do (print (add 8 (int 12)))(print \"Hello world!\")");
    astListPrint(head);
    puts("");
    second = Parser(head);
    astListPrint(second);
    puts("");
    item.list = second;
    astListAppend(&third, item, ASTNODE_LIST);
    ExecList(third);
    return 0;
}
