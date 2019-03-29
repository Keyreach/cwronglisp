#include <stdlib.h>
#include <stdio.h>
#include "vector.h"

#define CNAD(x, t) if(x == NULL){ puts(t); return; }
#define CNAR(x, t) if(x == NULL){ puts(t); return NULL; }

static unsigned int rwzr_allocations = 0;

vector
vector_new(size_t capacity){
    vector result = (vector)malloc(sizeof(struct vector)); rwzr_allocations++;
    result->capacity = capacity > 1 ? capacity : 1;
    result->size = 0;
    result->data = (TYPE*)malloc(sizeof(TYPE) * capacity);
    return result;
}

void
vector_init(vector v, size_t capacity){
	CNAD(v, "vector init: null pointer")
    v->capacity = capacity ? capacity : 1;
    v->size = 0;
    v->data = (TYPE*)malloc(sizeof(TYPE) * capacity);
}

void
vector_add(vector v, TYPE item){
	CNAD(v, "vector add: null pointer")
	CNAD(item, "vector add: null pointer -- item")
    if(v->size == v->capacity){
        v->data = realloc(v->data, sizeof(TYPE) * (v->capacity << 1));
        v->capacity = v->capacity << 1;
    }
    v->data[v->size] = item;
    v->size++;
}

TYPE
vector_get(vector v, size_t i){
	CNAR(v, "vector get: null pointer")
    if((i >= v->size) || (i < 0))
        return NULL;
    return rnode_copy(v->data[i]);
}

void
vector_set(vector v, size_t i, TYPE item){
	CNAD(v, "vector set: null pointer")
	CNAD(item, "vector set: null pointer -- item")
    if(i >= v->capacity){
        v->data = realloc(v->data, sizeof(TYPE) * (i + 1));
        v->capacity = i + 1;
    }
    if(i >= v->size) v->size = i + 1;
    v->data[i] = item;
}

void
vector_each(vector v, void(*f)(TYPE, int)){
	CNAD(v, "vector each: null pointer")
    int i, n = v->size;
    for(i = 0; i < n; i++){
        f(v->data[i], i);
    }
}

void
vector_print(vector v){
    int i;
    TYPE cdata;
    CNAD(v, "vector print: null pointer")
    printf("[ ");
    for(i = 0; i < v->size; i++){
        cdata = v->data[i];
        if(cdata->type == RWZR_TYPE_STRING){
            printf("\"%s\"%s ", cdata->data.str, i == v->size - 1 ? "" : ",");
        } else if(cdata->type == RWZR_TYPE_LIST) {
            vector_print(cdata->data.list);
            printf(i == v->size - 1 ? " " : ", ");
        } else if(cdata->type == RWZR_TYPE_SYMBOL) {
            printf("\"%s\"%s ", cdata->data.str, i == v->size - 1 ? "" : ",");
        } else if(cdata->type == RWZR_TYPE_NUMBER) {
            printf("\"%ld\"%s ", cdata->data.num, i == v->size - 1 ? "" : ",");
        } else if(cdata->type == RWZR_TYPE_FUNCTION){
            printf("<function>%s ", i == v->size - 1 ? "" : ",");
        } else {
            printf("<unknown type %d>%s ", cdata->type, i == v->size - 1 ? "" : ",");
        }
    }
    printf("] ");
}

void
vector_free(vector v){
    int i;
	CNAD(v, "vector free: null pointer")
    for(i = 0; i < v->size; i++){
        rnode_free(v->data[i]);
    }
    v->size = 0;
}

void
vector_destroy(vector v){
	CNAD(v, "vector destroy: null pointer");
	vector_free(v);
	free(v->data);
	free(v); rwzr_allocations--;
}

vector
vector_slice(vector v, size_t start, size_t end){
    int i, n;
	CNAR(v, "vector slice: null pointer")
    if(end == 0) n = v->size - start;
    else n = end - start;
    vector nv = vector_new(n);
    for(i = 0; i < n; i++){
        vector_set(nv, i, rnode_copy(v->data[start + i]));
    }
    return nv;
}

rwzr_value
rnode_text(char* s){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t)); rwzr_allocations++;
    value->type = RWZR_TYPE_STRING;
    value->data.str = s;
    printf("  created node: ");
	rnode_print(value);
    return value;
}

rwzr_value
rnode_sym(char* s){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t)); rwzr_allocations++;
    value->type = RWZR_TYPE_SYMBOL;
    value->data.str = s;
    return value;
}

rwzr_value
rnode_num(long int n){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t)); rwzr_allocations++;
    value->type = RWZR_TYPE_NUMBER;
    value->data.num = n;
    return value;
}

rwzr_value
rnode_list(vector v){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t)); rwzr_allocations++;
    value->type = RWZR_TYPE_LIST;
    value->data.list = v;
    return value;
}

rwzr_value
rnode_copy(rwzr_value v){
	CNAR(v, "rnode copy: null pointer")
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t)); rwzr_allocations++;
    value->type = v->type;
    switch(v->type){
    case RWZR_TYPE_LIST:
        value->data.list = vector_slice(v->data.list, 0, 0);
        break;
    case RWZR_TYPE_NUMBER:
        value->data.num = v->data.num;
        break;
    case RWZR_TYPE_STRING:
    case RWZR_TYPE_SYMBOL:
        value->data.str = v->data.str;
        break;
    default:
        value->data = v->data;
    }
    printf("  copied node: ");
	rnode_print(value);
    return value;
}

rwzr_value
rnode_func(vector params, vector body){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t)); rwzr_allocations++;
    value->type = RWZR_TYPE_FUNCTION;
    value->data.func = (rwzr_function)malloc(sizeof(rwzr_function_t)); rwzr_allocations++;
    value->data.func->params = params;
    value->data.func->body = body;
    return value;
}

void
rnode_free(rwzr_value r){
	CNAD(r, "rnode free: null pointer")
	printf("  freeing node: ");
	rnode_print(r);
    switch(r->type){
    case RWZR_TYPE_LIST:
        vector_destroy(r->data.list);
        break;
//    case RWZR_TYPE_FUNCTION:
//		vector_destroy(r->data.func->params);
//		vector_destroy(r->data.func->body);
//		break;
    }
    free(r); rwzr_allocations -= 1;
}

void rnode_print(rwzr_value r){
	CNAD(r, "rnode print: null pointer")
    if(r->type == RWZR_TYPE_STRING){
        printf("\"%s\"\n", r->data.str);
    } else if(r->type == RWZR_TYPE_LIST) {
        vector_print(r->data.list);
    } else if(r->type == RWZR_TYPE_SYMBOL) {
        printf("%s\n", r->data.str);
    } else if(r->type == RWZR_TYPE_NUMBER) {
        printf("%ld\n", r->data.num);
    } else if(r->type == RWZR_TYPE_FUNCTION){
        printf("<function>\n");
    } else {
        printf("<unknown type %d>\n", r->type);
    }
}

void print_allocations(){
	printf("debug: allocations: %d\n", rwzr_allocations);
}
