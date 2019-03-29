#include <stdio.h>
#include "vector.h"

vector func(vector w){
	vector v = vector_new(1);
	rwzr_value r = vector_get(w, 0);
	vector_add(v, rnode_text("hello"));
	vector_add(v, rnode_copy(r));
	rnode_free(r);
	vector_print(v); puts("in func");
	return v;
}

int main(){
	vector v = NULL;
	vector w = vector_new(1);
	vector_add(w, rnode_text("sample text"));
	puts(" >> function");
	v = func(w);
	puts(" << function");
	vector_print(v); puts("= v");
	vector_print(w); puts("= w");
	vector_destroy(v);
	vector_destroy(w);
	print_allocations();
	return 0;
}
