#include <stdlib.h>
#include <stdio.h>
#include "vector.h"

vector
vector_new(size_t capacity){
    vector result = (vector)malloc(sizeof(struct vector));
    result->capacity = capacity > 1 ? capacity : 1;
    result->size = 0;
    result->data = (TYPE*)malloc(sizeof(TYPE) * capacity);
    return result;
}

void
vector_init(vector v, size_t capacity){
    v->capacity = capacity ? capacity : 1;
    v->size = 0;
    v->data = (TYPE*)malloc(sizeof(TYPE) * capacity);
}

void
vector_add(vector v, TYPE item){
    if(v->size == v->capacity){
        v->data = realloc(v->data, sizeof(TYPE) * (v->capacity << 1));
        v->capacity = v->capacity << 1;
    }
    v->data[v->size] = item;
    v->size++;
}

TYPE
vector_get(vector v, size_t i){
    if((i >= v->size) || (i < 0))
        return NULL;
    return v->data[i];
}

void
vector_set(vector v, size_t i, TYPE item){
    if(i >= v->capacity){
        v->data = realloc(v->data, sizeof(TYPE) * (i + 1));
        v->capacity = i + 1;
    }
    if(i >= v->size) v->size = i + 1;
    v->data[i] = item;
}

void
vector_each(vector v, void(*f)(TYPE, int)){
    int i, n = v->size;
    for(i = 0; i < n; i++){
        f(v->data[i], i);
    }
}

void
vector_print(vector v){
    int i;
    TYPE cdata;
    printf("[ ");
    for(i = 0; i < v->size; i++){
        cdata = vector_get(v, i);
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
    if(v == NULL) return;
    for(i = 0; i < v->size; i++){
        rnode_free(v->data[i]);
    }
    v->size = 0;
}

vector
vector_slice(vector v, size_t start, size_t end){
    int i, n;
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
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    value->type = RWZR_TYPE_STRING;
    value->data.str = s;
    return value;
}

rwzr_value
rnode_sym(char* s){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    value->type = RWZR_TYPE_SYMBOL;
    value->data.str = s;
    return value;
}

rwzr_value
rnode_num(long int n){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    value->type = RWZR_TYPE_NUMBER;
    value->data.num = n;
    return value;
}

rwzr_value
rnode_list(vector v){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    value->type = RWZR_TYPE_LIST;
    value->data.list = v;
    return value;
}

rwzr_value
rnode_copy(rwzr_value v){
    if(v == NULL){
        return NULL;
    }
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    value->type = v->type;
    switch(v->type){
    case RWZR_TYPE_LIST:
        value->data.list = vector_slice(v->data.list, 0, 0);
        break;
    default:
        value->data = v->data;
    }
    return value;
}

rwzr_value
rnode_func(vector params, vector body){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    value->type = RWZR_TYPE_FUNCTION;
    value->data.func = (rwzr_function)malloc(sizeof(rwzr_function_t));
    vector_print(params);
    vector_print(body);
    value->data.func->params = params;
    value->data.func->body = body;
    return value;
}

void
rnode_free(rwzr_value r){
    if(r == NULL) return;
    switch(r->type){
    case RWZR_TYPE_LIST:
        vector_free(r->data.list);
        free(r->data.list);
    }
    //free(r);
}
