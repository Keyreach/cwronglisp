#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "astlist.h"

#define SWITCH_OPERATOR(x) if(strcmp(operator, x) == 0){
#define CASE_OPERATOR(x) } else if(strcmp(operator, x) == 0) {
#define CASE_DEFAULT } else {
#define CASE_END }

rwzr_list GlobalInterpreterScope = NULL;

/** key-value helpers **/
/*
ASTList PairListGet(ASTList *list, ASTList key){
    ASTList retval = astListFind(*list, key->value, ASTNODE_KEY);
    return retval->next;
}

void PairListSet(ASTList *list, ASTList key, ASTList value){
    ASTItem item;
    ASTList tmp, retval = astListFind(*list, key->value, ASTNODE_KEY);
    if(retval == NULL){
        item.str = key->value.str;
        astListAppend(list, item, ASTNODE_KEY);
        astListAppend(list, value->value, value->type);
    } else {
        // add replace to astlist.c
        tmp = retval->next->next;
        free(retval->next);
        retval->next = (ASTList)malloc(sizeof(ASTList_t));
        retval->next->next = tmp;
        retval->next->value = value->value;
        retval->next->type = value->type;
    }
}
*/
/** **/

char* Substring(char* text, size_t start, size_t end){
    size_t len = end - start;
    char* s = (char*)malloc(len + 1);
    memcpy(s, text + start, len);
    s[len] = '\0';
    return s;
}

rwzr_list rwzr_lexer(char *code){
    rwzr_list tokens = rlist_create();
    size_t tokenStart = -1;
    char c, quoted = 0;
    size_t i, n = strlen(code);
    for(i = 0; i < n; i++){
        c = code[i];
        if(quoted){
            if(c == '"'){
                rlist_push(
                    tokens,
                    rnode_text(Substring(code, tokenStart, i))
                );
                tokenStart = -1;
                quoted = 0;
            }
        } else {
            switch(c){
                case '(':
                case ')':
                    if(tokenStart != -1){
                        rlist_push(tokens, rnode_text(Substring(code, tokenStart, i)));
                        tokenStart = -1;
                    }
                    rlist_push(tokens, rnode_text(c == '(' ? "(" : ")"));
                    break;
                case '{':
                case '}':
                    if(tokenStart != -1){
                        rlist_push(tokens, rnode_text(Substring(code, tokenStart, i)));
                        tokenStart = -1;
                    }
                    rlist_push(tokens, rnode_text(c == '{' ? "(" : ")"));
                    if(c == '{'){
                        rlist_push(tokens, rnode_text("quote"));
                    }
                    break;
                case '"':
                    if(tokenStart != -1){
                        rlist_push(tokens, rnode_text(Substring(code, tokenStart, i)));
                        tokenStart = -1;
                    }
                    tokenStart = i + 1;
                    quoted = 1;
                    break;
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                    if(tokenStart != -1){
                        rlist_push(tokens, rnode_text(Substring(code, tokenStart, i)));
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
        rlist_push(tokens, rnode_text(Substring(code, tokenStart, i)));
        tokenStart = -1;
    }
    return tokens;
}

rwzr_list rwzr_parser(rwzr_list tokens){
    rwzr_list tree = rlist_create();
    rwzr_list branch = rlist_create();
    char *token;
    size_t brackets = 0;
    size_t i, n = rlist_len(tokens);
    for(i = 0; i < n; i++){
        token = rlist_get(tokens, i)->data.str;
        if(strcmp(token, "(") == 0){
            if(brackets > 0){
                rlist_push(branch, rnode_text(token));
            }
            brackets++;
        } else if(strcmp(token, ")") == 0) {
            brackets--;
            if(brackets > 0){
                rlist_push(branch, rnode_text(token));
            } else {
                rlist_push(tree, rnode_list(rwzr_parser(branch)));
                rlist_empty(branch);
            }
        } else {
            if(brackets > 0){
                rlist_push(branch, rnode_text(token));
            } else {
                rlist_push(tree, rnode_text(token));
            }
        }
    }
    return tree;
}

/*

ASTList ExecList(ASTList ast){
    char* operator;
    ASTList a, b, tmp, cond, retval;
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
        return NewNumNode(strtol(a->value.str, NULL, 10));

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
        return NewListNode(astListSlice(ast->value.list, 1, 0));

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
        PairListSet(&GlobalInterpreterScope, a, b);
        return b;

    CASE_OPERATOR("get")
        a = ExecList(astListGet(ast->value.list, 1));
        if(a == NULL){
            puts("get: nullptr exception");
            return NULL;
        } else if(a->type != ASTNODE_STR){
            puts("get: wrong operands type");
            return NULL;
        }
        return PairListGet(&GlobalInterpreterScope, a);

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
        return NewNumNode(a->value.num + b->value.num);

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
        return NewNumNode(a->value.num - b->value.num);

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
        return NewNumNode(a->value.num * b->value.num);

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
        return NewNumNode(a->value.num / b->value.num);

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
        return NewNumNode(a->value.num % b->value.num);

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
        return NewNumNode(a->value.num < b->value.num);

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
        return NewNumNode(a->value.num > b->value.num);

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
        return NewNumNode(a->value.num == b->value.num);

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
        return NewNumNode(a->value.num != b->value.num);

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

*/

int main(){
    rwzr_list head, second, third;
    size_t bytes_read = 0;
    char *buffer = (char*)malloc(1024);
    fread(buffer, 1, 1024, stdin);
    puts(buffer);
    head = rwzr_lexer(buffer);
    //head = Tokenize("do (set x (int 1)) (while (lt (get x) (int 99)) (do (print (get x)) (set x (mul (get x) (int 2))) ))");
    puts("Lexer output:");
    rlist_print(head);
    puts("\nParser output:");
    second = rwzr_parser(head);
    rlist_print(second);
    /*
    puts("\nInterpreter output:");
    item.list = second;
    astListAppend(&third, item, ASTNODE_LIST);
    */
    // ExecList(third);
    return 0;
}
