#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "astlist.h"

#define SWITCH_OPERATOR(x) if(strcmp(operator, x) == 0){
#define CASE_OPERATOR(x) } else if(strcmp(operator, x) == 0) {
#define CASE_DEFAULT } else {
#define CASE_END }

rwzr_list GlobalInterpreterScope;

/** key-value helpers **/

rwzr_value pairlist_get(rwzr_list list, char* key){
    rwzr_value result, keynode = (rwzr_value)rnode_sym(key);
    rwzr_node retval = rlist_find_node(list, keynode);
    free(keynode);
    result = (rwzr_value)(retval->next->data);
    return result;
}

void pairlist_set(rwzr_list list, char* key, rwzr_value value){
    rwzr_value keynode = (rwzr_value)rnode_sym(key);
    rwzr_node retval = rlist_find_node(list, keynode);
    if(retval == NULL){
        rlist_push(list, (void*)keynode);
        rlist_push(list, (void*)rnode_copy(value));
    } else {
        // add replace to astlist.c
        free(keynode);
        free(retval->next->data);
        retval->next->data = (void*)rnode_copy(value);
    }
}

/** **/

char* Substring(char* text, size_t start, size_t end){
    size_t len = end - start;
    char* s = (char*)malloc(len + 1);
    memcpy(s, text + start, len);
    s[len] = '\0';
    return s;
}

rwzr_list
rwzr_lexer(char *code){
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

rwzr_list
rwzr_parser(rwzr_list tokens){
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

rwzr_value
exec(rwzr_value ast){
    char* operator;
    rwzr_list ops;
    rwzr_node temp_node;
    rwzr_value a, b, cond, retval; // <- must rnode_free them;
    if(ast->type == RWZR_TYPE_STRING){
        return (rwzr_value)rnode_copy(ast);
    }
    ops = ast->data.list;
    operator = exec(rlist_get(ops, 0))->data.str;
    SWITCH_OPERATOR("int")
        a = exec(rlist_get(ops, 1));
        if(a == NULL){
            puts("int: nullptr exception");
            return NULL;
        } else if(a->type != RWZR_TYPE_STRING){
            puts("int: wrong operands type");
            return NULL;
        }
        return rnode_num(strtol(a->data.str, NULL, 10));

    CASE_OPERATOR("do")
        retval = NULL;
        temp_node = ops->nodes->next;
        while(temp_node != NULL){
            if(retval != NULL) free(retval);
            retval = exec((rwzr_value)(temp_node->data));
            temp_node = temp_node->next;
        }
        return retval;

    CASE_OPERATOR("quote")
        return rnode_list(rlist_slice(ops, 1, 0));

    CASE_OPERATOR("set")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if((a == NULL) || (b == NULL)){
            puts("set: nullptr exception");
            return NULL;
        } else if(a->type != RWZR_TYPE_STRING){
            puts("set: wrong operands type");
            return NULL;
        }
        pairlist_set(GlobalInterpreterScope, a->data.str, b);
        return b;

    CASE_OPERATOR("get")
        a = exec(rlist_get(ops, 1));
        if(a == NULL){
            puts("get: nullptr exception");
            return NULL;
        } else if(a->type != RWZR_TYPE_STRING){
            puts("get: wrong operands type");
            return NULL;
        }
        return pairlist_get(GlobalInterpreterScope, a->data.str);

    CASE_OPERATOR("print")
        a = exec(rlist_get(ops, 1));
        if(a == NULL){
            puts("print: nullptr exception");
            return NULL;
        } else if(a->type == RWZR_TYPE_STRING){
            printf("%s\n", a->data.str);
        } else if(a->type == RWZR_TYPE_NUMBER){
            printf("%ld\n", a->data.num);
        } else if(a->type == RWZR_TYPE_LIST){
            rlist_print(a->data.list);
            printf("\n");
        } else {
            printf("print: wrong operand type %d\n", a->type);
        }
        return NULL;

    CASE_OPERATOR("add")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
            return NULL;
        }
        return rnode_num(a->data.num + b->data.num);

    CASE_OPERATOR("sub")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
            return NULL;
        }
        return rnode_num(a->data.num - b->data.num);

    CASE_OPERATOR("mul")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
            return NULL;
        }
        return rnode_num(a->data.num * b->data.num);

    CASE_OPERATOR("div")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
            return NULL;
        }
        return rnode_num(a->data.num / b->data.num);

    CASE_OPERATOR("mod")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if(a == NULL){
            puts("add: nullptr exception");
            return NULL;
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
            return NULL;
        }
        return rnode_num(a->data.num % b->data.num);

    CASE_OPERATOR("lt")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if(a == NULL){
            puts("lt: nullptr exception");
            return NULL;
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("lt: wrong operands type");
            return NULL;
        }
        return rnode_num(a->data.num < b->data.num);

    CASE_OPERATOR("gt")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if(a == NULL){
            puts("gt: nullptr exception");
            return NULL;
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("gt: wrong operands type");
            return NULL;
        }
        return rnode_num(a->data.num > b->data.num);

    CASE_OPERATOR("eq")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if(a == NULL){
            puts("eq: nullptr exception");
            return NULL;
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("eq: wrong operands type");
            return NULL;
        }
        return rnode_num(a->data.num == b->data.num);

    CASE_OPERATOR("ne")
        a = exec(rlist_get(ops, 1));
        b = exec(rlist_get(ops, 2));
        if(a == NULL){
            puts("ne: nullptr exception");
            return NULL;
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("ne: wrong operands type");
            return NULL;
        }
        return rnode_num(a->data.num != b->data.num);

    CASE_OPERATOR("while")
        a = rlist_get(ops, 1);
        b = rlist_get(ops, 2);
        cond = exec(a);
        while((cond != NULL) && (cond->type == RWZR_TYPE_NUMBER) && (cond->data.num != 0)){
            exec(b);
            cond = exec(a);
        }

    CASE_DEFAULT
        printf("Unknown operator: %s\n", operator);

    CASE_END
    return NULL;
}


int main(){
    rwzr_list head, second;
    GlobalInterpreterScope = rlist_create();
    char *buffer = (char*)malloc(1024);
    fread(buffer, 1, 1024, stdin);
    puts(buffer);
    head = rwzr_lexer(buffer);
    //head = Tokenize("do (set x (int 1)) (while (lt (get x) (int 99)) (do (print (get x)) (set x (mul (get x) (int 2))) ))");
    puts("Lexer output:");
    rlist_print(head);
    puts("\nParser output:");
    second = rwzr_parser(head);
    rlist_empty(head);
    rlist_print(second);
    puts("\nInterpreter output:");
    exec(rnode_list(second));
    rlist_empty(second);
    printf("= %ld\n", rnode_allocs());
    return 0;
}
