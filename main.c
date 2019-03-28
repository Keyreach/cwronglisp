#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"

#define SWITCH_OPERATOR(x) if(strcmp(operator, x) == 0){
#define CASE_OPERATOR(x) } else if(strcmp(operator, x) == 0) {
#define CASE_DEFAULT } else {
#define CASE_END }

#define RWZR_EXEC_KEEP_AST 0x1

rwzr_value pairlist_get(vector list, char* key);
void       pairlist_set(vector list, char* key, rwzr_value value);
char*      Substring(char* text, size_t start, size_t end);
rwzr_value exec(rwzr_value ast, vector ctx, int flags);
vector      rwzr_lexer(char *code);
vector      rwzr_parser(vector tokens);
rwzr_value  func_call(vector arguments, vector ctx);

vector  global_context;

/** key-value helpers **/

rwzr_value
pairlist_get(vector v, char* key){
    int i;
    printf("key: %s\n", key);
    vector_print(v);
    puts("context");
    for(i = 0; i < v->size; i++){
        if((v->data[i]->type == RWZR_TYPE_SYMBOL) && (strcmp(key, v->data[i]->data.str) == 0)){
            return rnode_copy(v->data[i + 1]);
        }
    }
    puts("lookup in parent");
    for(i = 0; i < v->size; i++){
        if((v->data[i]->type == RWZR_TYPE_SYMBOL) && (strcmp(key, "__parent") == 0)){
            return pairlist_get(v->data[i + 1]->data.list, key);
        }
    }   
    return NULL;
}

void
pairlist_set(vector v, char* key, rwzr_value value){
    int i;
    for(i = 0; i < v->size; i++){
        if((v->data[i]->type == RWZR_TYPE_SYMBOL) && (strcmp(key, v->data[i]->data.str) == 0)){
            rnode_free(v->data[i + 1]);
            v->data[i + 1] = rnode_copy(value);
            return;
        }
    }
    vector_add(v, rnode_sym(key));
    vector_add(v, rnode_copy(value));
}

/** **/

char*
Substring(char* text, size_t start, size_t end){
    size_t len = end - start;
    char* s = (char*)malloc(len + 1);
    memcpy(s, text + start, len);
    s[len] = '\0';
    return s;
}

vector
rwzr_lexer(char *code){
    vector tokens = vector_new(1);
    size_t tokenStart = -1;
    char c, quoted = 0;
    size_t i, n = strlen(code);
    for(i = 0; i < n; i++){
        c = code[i];
        if(quoted){
            if(c == '"'){
                vector_add(
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
                        vector_add(tokens, rnode_text(Substring(code, tokenStart, i)));
                        tokenStart = -1;
                    }
                    vector_add(tokens, rnode_text(c == '(' ? "(" : ")"));
                    break;
                case '{':
                case '}':
                    if(tokenStart != -1){
                        vector_add(tokens, rnode_text(Substring(code, tokenStart, i)));
                        tokenStart = -1;
                    }
                    vector_add(tokens, rnode_text(c == '{' ? "(" : ")"));
                    if(c == '{'){
                        vector_add(tokens, rnode_text("quote"));
                    }
                    break;
                case '"':
                    if(tokenStart != -1){
                        vector_add(tokens, rnode_text(Substring(code, tokenStart, i)));
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
                        vector_add(tokens, rnode_text(Substring(code, tokenStart, i)));
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
        vector_add(tokens, rnode_text(Substring(code, tokenStart, i)));
        tokenStart = -1;
    }
    return tokens;
}

vector
rwzr_parser(vector tokens){
    vector tree = vector_new(1);
    struct vector branch;
    vector_init(&branch, 1);
    char *token;
    size_t brackets = 0;
    size_t i, n = tokens->size;
    for(i = 0; i < n; i++){
        token = vector_get(tokens, i)->data.str;
        if(strcmp(token, "(") == 0){
            if(brackets > 0){
                vector_add(&branch, rnode_text(token));
            }
            brackets++;
        } else if(strcmp(token, ")") == 0) {
            brackets--;
            if(brackets > 0){
                vector_add(&branch, rnode_text(token));
            } else {
                vector_add(tree, rnode_list(rwzr_parser(&branch)));
            }
        } else {
            if(brackets > 0){
                vector_add(&branch, rnode_text(token));
            } else {
                vector_add(tree, rnode_text(token));
            }
        }
    }
    vector_free(tokens);
    return tree;
}

rwzr_value
func_call(vector arguments, vector ctx){
    int i;
    rwzr_value result, tmp, func_name = vector_get(arguments, 0);
    if((func_name == NULL) || (func_name->type != RWZR_TYPE_STRING)){
        puts("call: invalid identifier");
        return NULL;
    }
    rwzr_value func_data = pairlist_get(ctx, func_name->data.str);
    if((func_data == NULL) || (func_data->type != RWZR_TYPE_FUNCTION) || (func_data->data.func == NULL) || (func_data->data.func->body == NULL) || (func_data->data.func->params == NULL)){
        puts("call: no function");
        return NULL;
    }
    rwzr_function func = func_data->data.func;
    struct vector new_scope;
    vector_init(&new_scope, 1);
    vector_add(&new_scope, rnode_sym("__parent"));
    vector_add(&new_scope, rnode_list(ctx));
    for(i = 0; i < arguments->size - 1; i++){
        tmp = vector_get(func->params, i);
        tmp->type = RWZR_TYPE_SYMBOL;
        vector_add(&new_scope, tmp);
        vector_add(&new_scope, exec(vector_get(arguments, i + 1), ctx, 0));
    }
    result = exec(rnode_list(func->body), &new_scope, RWZR_EXEC_KEEP_AST);
    return result;
}

rwzr_value
exec(rwzr_value ast, vector ctx, int flags){
    int i;
    char* operator;
    vector ops = NULL;
    rwzr_value a = NULL, b = NULL, cond = NULL, result = NULL; // <- must rnode_free them or invent better approach to temp vars
    if(ast->type == RWZR_TYPE_STRING){
        result = rnode_copy(ast);
        rnode_print(ast);
        return result;
    }
    ops = ast->data.list;
    operator = exec(vector_get(ops, 0), ctx, 0)->data.str;
    printf("operator: %s\n", operator);
    vector_print(ops);
    puts("");
    SWITCH_OPERATOR("int")
        a = exec(vector_get(ops, 1), ctx, 0);
        if(a == NULL){
            puts("int: nullptr exception");
        } else if(a->type != RWZR_TYPE_STRING){
            puts("int: wrong operands type");
            return NULL;
        } else {
			result = rnode_num(strtol(a->data.str, NULL, 10));
		}

    CASE_OPERATOR("do")
        for(i = 1; i < ops->size; i++){
            result = exec(ops->data[i], ctx, 0);
        }

    CASE_OPERATOR("quote")
        result = rnode_list(vector_slice(ops, 1, 0));

    CASE_OPERATOR("set")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if((a == NULL) || (b == NULL)){
            puts("set: nullptr exception");
        } else if(a->type != RWZR_TYPE_STRING){
            puts("set: wrong operands type");
        } else {
			pairlist_set(ctx, a->data.str, b);
			result = rnode_copy(b);
		}

    CASE_OPERATOR("get")
        a = exec(vector_get(ops, 1), ctx, 0);
        if(a == NULL){
            puts("get: nullptr exception");
        } else if(a->type != RWZR_TYPE_STRING){
            puts("get: wrong operands type");
        } else {
			result = pairlist_get(ctx, a->data.str);
		}

    CASE_OPERATOR("print")
        a = exec(vector_get(ops, 1), ctx, 0);
        if(a == NULL){
            puts("print: nullptr exception");
        } else if(a->type == RWZR_TYPE_STRING){
            printf("%s\n", a->data.str);
        } else if(a->type == RWZR_TYPE_NUMBER){
            printf("%ld\n", a->data.num);
        } else if(a->type == RWZR_TYPE_LIST){
            vector_print(a->data.list);
            printf("\n");
        } else {
            printf("print: wrong operand type %d\n", a->type);
        }

    CASE_OPERATOR("add")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if(a == NULL){
            puts("add: nullptr exception");
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
            printf("%d ~ %d\n", a->type, b->type);
        } else {
			result = rnode_num(a->data.num + b->data.num);
		}

    CASE_OPERATOR("sub")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if(a == NULL){
            puts("add: nullptr exception");
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
        } else {
			return rnode_num(a->data.num - b->data.num);
		}

    CASE_OPERATOR("mul")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if(a == NULL){
            puts("add: nullptr exception");
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
        } else {
			result = rnode_num(a->data.num * b->data.num);
		}

    CASE_OPERATOR("div")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if(a == NULL){
            puts("add: nullptr exception");
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
        } else {
			result = rnode_num(a->data.num / b->data.num);
		}

    CASE_OPERATOR("mod")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if(a == NULL){
            puts("add: nullptr exception");
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("add: wrong operands type");
        } else {
			result = rnode_num(a->data.num % b->data.num);
		}

    CASE_OPERATOR("lt")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if(a == NULL){
            puts("lt: nullptr exception");
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("lt: wrong operands type");
        } else {
			result = rnode_num(a->data.num < b->data.num);
		}

    CASE_OPERATOR("gt")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if(a == NULL){
            puts("gt: nullptr exception");
        } else if((a->type != RWZR_TYPE_NUMBER) || (b->type != RWZR_TYPE_NUMBER)){
            puts("gt: wrong operands type");
        } else {
			result = rnode_num(a->data.num > b->data.num);
		}

    CASE_OPERATOR("eq")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if(a == NULL){
            puts("eq: nullptr exception");
        } else if((a->type == RWZR_TYPE_NUMBER) && (b->type == RWZR_TYPE_NUMBER)){
			result = rnode_num(a->data.num == b->data.num);
		} else if((a->type == RWZR_TYPE_STRING) && (b->type == RWZR_TYPE_STRING)){
			result = rnode_num(strcmp(a->data.str, b->data.str) == 0);
		} else {
            puts("eq: wrong operands type");
        }
        
    CASE_OPERATOR("ne")
        a = exec(vector_get(ops, 1), ctx, 0);
        b = exec(vector_get(ops, 2), ctx, 0);
        if(a == NULL){
            puts("ne: nullptr exception");
        } else if((a->type == RWZR_TYPE_NUMBER) && (b->type == RWZR_TYPE_NUMBER)){
			result = rnode_num(a->data.num != b->data.num);
		} else if((a->type == RWZR_TYPE_STRING) && (b->type == RWZR_TYPE_STRING)){
			result = rnode_num(strcmp(a->data.str, b->data.str) != 0);
        } else {
            puts("ne: wrong operands type");
        }

    CASE_OPERATOR("while")
        a = vector_get(ops, 1);
        b = vector_get(ops, 2);
        if((a == NULL) || (b == NULL)){
			puts("while: nullptr exception");
		} else {
			cond = exec(a, ctx, RWZR_EXEC_KEEP_AST);
			if(cond == NULL){
				puts("while: nullptr exception");
			} else if(cond->type != RWZR_TYPE_NUMBER){
				puts("while: type missmatch");
			} else {
				while(cond->data.num != 0){
					result = exec(b, ctx, RWZR_EXEC_KEEP_AST);
					cond = exec(a, ctx, RWZR_EXEC_KEEP_AST); // check condition here
				}
			}
		}

    CASE_OPERATOR("func")
        a = vector_get(ops, 1);
        b = vector_get(ops, 2);
        vector_print(a->data.list);
        vector_print(b->data.list);
        result = rnode_func(
            vector_slice(a->data.list, 0, 0),
            vector_slice(b->data.list, 0, 0)
        );

    CASE_OPERATOR("call")
        result = func_call(vector_slice(ops, 1, 0), ctx);

    CASE_DEFAULT
        printf("Unknown operator: %s\n", operator);

    CASE_END
    if(a != NULL) rnode_free(a);
    if(b != NULL) rnode_free(b);
    if(cond != NULL) rnode_free(cond);
    if(!(flags|RWZR_EXEC_KEEP_AST))
		rnode_free(ast);
    return result;
}

int main(){
    vector tokens, syntax_tree;
    global_context = vector_new(1);
    char *buffer = (char*)malloc(1024);
    fread(buffer, 1, 1024, stdin);
    puts(buffer);
    tokens = rwzr_lexer(buffer);
    puts("Lexer output:");
    vector_print(tokens);
    puts("\nParser output:");
    syntax_tree = rwzr_parser(tokens);
    vector_print(syntax_tree);
    free(tokens);
    puts("\nInterpreter output:");
    exec(rnode_list(syntax_tree), global_context, 0);
    /* vector_free(syntax_tree);
    if(syntax_tree != NULL) free(syntax_tree); */
    vector_free(global_context);
    if(global_context != NULL) free(global_context);
    return 0;
}
