#define TYPE rwzr_value

#define RWZR_TYPE_NIL 0
#define RWZR_TYPE_STRING 1
#define RWZR_TYPE_LIST 2
#define RWZR_TYPE_NUMBER 3
#define RWZR_TYPE_SYMBOL 4
#define RWZR_TYPE_FUNCTION 5

typedef union rwzr_data {
    char *str;
    long num;
    struct vector *list;
    struct rwzr_function *func;
} rwzr_data;

typedef struct rwzr_function {
    struct vector* params;
    struct vector* body;
} rwzr_function_t;
typedef rwzr_function_t* rwzr_function;

typedef struct rwzr_value {
    char type;
    rwzr_data data;
} rwzr_value_t;
typedef rwzr_value_t* rwzr_value;

struct vector {
    TYPE* data;
    size_t capacity;
    size_t size;
};
typedef struct vector* vector;

vector  vector_new(size_t capacity);
void    vector_init(vector v, size_t capacity);
void    vector_add(vector v, TYPE item);
TYPE    vector_get(vector v, size_t i);
void    vector_set(vector v, size_t i, TYPE item);
void    vector_each(vector v, void(*f)(TYPE, int));
void    vector_print(vector v);
void    vector_free(vector v);
vector  vector_slice(vector v, size_t start, size_t end);
void	vector_destroy(vector v);

rwzr_value  rnode_text(char * s);
rwzr_value  rnode_list(vector v);
rwzr_value  rnode_num(long int n);
rwzr_value  rnode_sym(char * s);
rwzr_value  rnode_func(vector params, vector body);
rwzr_value  rnode_copy(rwzr_value v);
void        rnode_free(rwzr_value r);
void        rnode_print(rwzr_value r);

void print_allocations();
