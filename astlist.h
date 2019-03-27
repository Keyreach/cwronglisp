#define RWZR_TYPE_NIL 0
#define RWZR_TYPE_STRING 1
#define RWZR_TYPE_LIST 2
#define RWZR_TYPE_NUMBER 3
#define RWZR_TYPE_SYMBOL 4

typedef struct rwzr_node {
    void *data;
    struct rwzr_node *next;
} rwzr_node_t;
typedef rwzr_node_t* rwzr_node;

typedef struct rwzr_list {
    rwzr_node cursor;
    rwzr_node nodes;
} rwzr_list_t;
typedef rwzr_list_t* rwzr_list;

typedef union rwzr_data {
    char *str;
    long num;
    struct rwzr_list *list;
} rwzr_data;

typedef struct rwzr_value {
    char type;
    rwzr_data data;
} rwzr_value_t;
typedef rwzr_value_t* rwzr_value;

rwzr_list rlist_create();
void      rlist_push(rwzr_list list, void* data);
int       rlist_end(rwzr_list list);
rwzr_node rlist_next(rwzr_list list);
// to be done
size_t    rlist_len(rwzr_list list);
rwzr_value rlist_get(rwzr_list list, size_t index);
rwzr_node rlist_get_node(rwzr_list list, size_t index);
void      rlist_delete(rwzr_list list, size_t index);
rwzr_list rlist_slice(rwzr_list list, size_t start, size_t end);
void      rlist_print(rwzr_list list);
void      rlist_empty(rwzr_list list);
rwzr_node rlist_find_node(rwzr_list haystack, rwzr_value needle);
// node constructors
void*     rnode_text(char* s);
void*     rnode_num(long int x);
void*     rnode_list(rwzr_list list);
void*     rnode_sym(char* s);
void*     rnode_copy(rwzr_value val);
void      rnode_free(rwzr_value data);
//
long int  rnode_allocs();
