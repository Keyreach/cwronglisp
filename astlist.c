#include <stdio.h>
#include <stdlib.h>
#include "astlist.h"

size_t astListLen(ASTList **list){
    size_t length = 0;
    ASTList *current = *list;
    while(current != NULL){
        length++;
        current = current->next;
    }
    return length;
}

ASTList* astListGet(ASTList *list, size_t index){
    size_t counter = 0;
    ASTList *current = list;
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

void astListDelete(ASTList **list, size_t index){
    size_t counter = 0;
    ASTList *tmp = NULL, *current = *list;
    if(index == 0){
        *list = current->next;
        return;
    }
    while((counter < index - 1) && (current != NULL)){
        counter++;
        current = current->next;
    }
    if((current != NULL) && (current->next != NULL)){
        tmp = current->next->next;
        free(current->next);
        current->next = tmp;
    }
}

void astListAppend(ASTList **list, ASTItem item, unsigned int type){
    ASTList *newnode, *current = NULL;
    if(*list == NULL){
        current = (ASTList*)malloc(sizeof(ASTList));
        current->type = type;
        current->value = item;
        current->next = NULL;
        *list = current;
    } else {
        current = *list;
        while(current->next != NULL){
            current = current->next;
        }
        newnode = (ASTList*)malloc(sizeof(ASTList));
        newnode->type = type;
        newnode->value = item;
        newnode->next = NULL;
        current->next = newnode;
    }
}

void astListPrint(ASTList *list){
    ASTList *current = list;
    printf("[ ");
    while(current != NULL){
        if(current->type == ASTNODE_STR){
            printf("\"%s\"%s ", current->value.str, current->next == NULL ? "" : ",");
        } else if(current->type == ASTNODE_LIST) {
            astListPrint(current->value.list);
        }
        current = current->next;
    }
    printf("] ");
}

void astListEmpty(ASTList **list){
    size_t len = astListLen(list) - 1;
    while(len > 0){
        astListDelete(list, len);
        len--;
    }
    *list = NULL;
}

ASTList* astListSlice(ASTList *list, size_t start, size_t end){
    ASTList* result = NULL, *head, *item;
    size_t counter = start;
    head = astListGet(list, start);
    while((head != NULL) && ((end == 0) || (counter < end))){
        switch(head->type){
            case ASTNODE_LIST:
                astListAppend(&result, head->value, head->type);
                break;
            case ASTNODE_STR:
                astListAppend(&result, head->value, head->type);
                break;
            case ASTNODE_NUM:
                astListAppend(&result, head->value, head->type);
                break;
        }
        head = head->next;
        counter++;
    }
    return result;
}
