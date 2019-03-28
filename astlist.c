#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "astlist.h"

static long int RWZR_MALLOCS = 0;

rwzr_list
rlist_create(){
    rwzr_list list = (rwzr_list)malloc(sizeof(rwzr_list_t));
    //RWZR_MALLOCS++;
    list->nodes = NULL;
    list->cursor = NULL;
    return list;
}

void
rlist_push(rwzr_list list, void* data){
    rwzr_node cur, node = (rwzr_node)malloc(sizeof(rwzr_node_t));
    RWZR_MALLOCS++;
    node->data = data;
    node->next = NULL;
    if(list->nodes == NULL){
        list->nodes = node;
        list->cursor = node;
        return;
    } else {
        cur = list->nodes;
        while(cur->next != NULL){ cur = cur->next; }
        cur->next = node;
    }
}

int
rlist_end(rwzr_list list){
    return list->cursor == NULL;
}

rwzr_node
rlist_next(rwzr_list list){
    rwzr_node tmp;
    if(list->cursor == NULL) return NULL;
    tmp = list->cursor;
    list->cursor = list->cursor->next;
    return tmp;
}

rwzr_value
rlist_next_value(rwzr_list list){
    rwzr_node tmp;
    if(list->cursor == NULL) return NULL;
    tmp = list->cursor;
    list->cursor = list->cursor->next;
    return ((rwzr_value)(tmp->data));
}

void
rlist_rewind(rwzr_list list){
    list->cursor = list->nodes;
}

size_t
rlist_len(rwzr_list list){
    size_t length = 0;
    rwzr_node current = list->nodes;
    while(current != NULL){
        length++;
        current = current->next;
    }
    return length;
}

rwzr_value
rlist_get(rwzr_list list, size_t index){
    size_t counter = 0;
    rwzr_node current = list->nodes;
    while((counter < index) && (current != NULL)){
        counter++;
        current = current->next;
    }
    if(current != NULL){
        return (rwzr_value)(current->data);
    } else {
        return NULL;
    }
}

rwzr_node
rlist_get_node(rwzr_list list, size_t index){
    size_t counter = 0;
    rwzr_node current = list->nodes;
    while((counter < index) && (current != NULL)){
        counter++;
        current = current->next;
    }
    if(current != NULL){
        return current;
    } else {
        return NULL;
    }
}

void
rlist_delete(rwzr_list list, size_t index){
    size_t counter = 0;
    rwzr_node tmp = NULL, current = list->nodes;
    if(index == 0){
        tmp = current;
        list->nodes = current->next;
        rnode_free(tmp->data);
        free(tmp);
        RWZR_MALLOCS -= 1;
        return;
    }
    while((counter < index - 1) && (current != NULL)){
        counter++;
        current = current->next;
    }
    if((current != NULL) && (current->next != NULL)){
        tmp = current->next->next;
        rnode_free(current->next->data);
        free(current->next);
        RWZR_MALLOCS -= 1;
        current->next = tmp;
    }
}

void
rlist_print(rwzr_list list){
    rwzr_node current = list->nodes;
    rwzr_value cdata;
    printf("[ ");
    while(current != NULL){
        cdata = ((rwzr_value)(current->data));
        if(cdata->type == RWZR_TYPE_STRING){
            printf("\"%s\"%s ", cdata->data.str, current->next == NULL ? "" : ",");
        } else if(cdata->type == RWZR_TYPE_LIST) {
            rlist_print(cdata->data.list);
            printf(current->next == NULL ? " " : ", ");
        } else if(cdata->type == RWZR_TYPE_SYMBOL) {
            printf("\"%s\"%s ", cdata->data.str, current->next == NULL ? "" : ",");
        } else if(cdata->type == RWZR_TYPE_NUMBER) {
            printf("\"%ld\"%s ", cdata->data.num, current->next == NULL ? "" : ",");
        } else if(cdata->type == RWZR_TYPE_FUNCTION){
            printf("<function>%s ", current->next == NULL ? "" : ",");
        } else {
            printf("<unknown type %d>%s ", cdata->type, current->next == NULL ? "" : ",");
        }
        current = current->next;
    }
    printf("] ");
}

void
rlist_empty(rwzr_list list){
    rwzr_node current;
    while(!rlist_end(list)){
        current = rlist_next(list);
        rnode_free(current->data);
        free(current);
        RWZR_MALLOCS -= 1;
    }
    list->nodes = NULL;
}

rwzr_list
rlist_slice(rwzr_list list, size_t start, size_t end){
    rwzr_list result = rlist_create();
    rwzr_node head;
    size_t counter = start;
    head = rlist_get_node(list, start);
    while((head != NULL) && ((end == 0) || (counter < end))){
        rlist_push(result, rnode_copy(head->data));
        head = head->next;
        counter++;
    }
    return result;
}

rwzr_node
rlist_find_node(rwzr_list haystack, rwzr_value needle){
    rwzr_node current = haystack->nodes;
    rwzr_value cdata;
    if((needle->type != RWZR_TYPE_STRING) && (needle->type != RWZR_TYPE_SYMBOL) && (needle->type != RWZR_TYPE_NUMBER)){
        puts("Incompatible comparison");
        printf("%d\n", needle->type);
        return NULL;
    }
    while(current != NULL){
        cdata = ((rwzr_value)(current->data));
        if(cdata->type != needle->type){
            current = current->next; continue;
        }
        if((needle->type == RWZR_TYPE_STRING) && (strcmp(needle->data.str, cdata->data.str) == 0)){
            return current;
        }
        if((needle->type == RWZR_TYPE_SYMBOL) && (strcmp(needle->data.str, cdata->data.str) == 0)){
            return current;
        }
        if((needle->type == RWZR_TYPE_NUMBER) && (cdata->data.num == needle->data.num)) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void*
rnode_text(char* s){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    RWZR_MALLOCS++;
    value->type = RWZR_TYPE_STRING;
    value->data.str = s;
    return value;
}
void*
rnode_num(long int x){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    RWZR_MALLOCS++;
    value->type = RWZR_TYPE_NUMBER;
    value->data.num = x;
    return value;
}

void*
rnode_list(rwzr_list list){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    RWZR_MALLOCS++;
    value->type = RWZR_TYPE_LIST;
    value->data.list = list;
    return value;
}

void*
rnode_sym(char* s){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    RWZR_MALLOCS++;
    value->type = RWZR_TYPE_SYMBOL;
    value->data.str = s;
    return value;
}

void*
rnode_func(rwzr_list params, rwzr_list body){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    RWZR_MALLOCS++;
    value->type = RWZR_TYPE_FUNCTION;
    value->data.func = (rwzr_function)malloc(sizeof(rwzr_function_t));
    value->data.func->params = params;
    value->data.func->body = body;
    return value;
}

void*
rnode_copy(rwzr_value val){
    rwzr_value value = (rwzr_value)malloc(sizeof(rwzr_value_t));
    RWZR_MALLOCS++;
    value->type = val->type;
    value->data = val->data;
    return value;
}

void
rnode_free(rwzr_value data){
    switch(data->type){
        case RWZR_TYPE_LIST:
            rlist_empty(data->data.list);
            break;
        /** DON'T FREE STRINGS AS THEY'RE NOT CLONED DURING STAGES **/
    }
    RWZR_MALLOCS -= 1;
    free(data);
}

long int
rnode_allocs(){
    return RWZR_MALLOCS;
}
