#define ASTNODE_STR 0
#define ASTNODE_LIST 1
#define ASTNODE_NUM 2

typedef union ASTItem {
    char *str;
    long num;
    struct ASTList *list;
} ASTItem;

typedef struct ASTList {
    char type;
    ASTItem value;
    struct ASTList *next;
} ASTList;

static ASTItem NilItem;

size_t astListLen(ASTList **list);
ASTList* astListGet(ASTList *list, size_t index);
void astListDelete(ASTList **list, size_t index);
void astListAppend(ASTList **list, ASTItem item, unsigned int type);
ASTList* astListSlice(ASTList *list, size_t start, size_t end);
void astListPrint(ASTList *list);
void astListEmpty(ASTList **list);
ASTList* astListFind(ASTList *haystack, ASTItem needle, unsigned int type);
