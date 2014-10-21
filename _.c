#include "_.h"


static void *default_alloc(void *old, size_t size) {
	if (!old) {
		return calloc(1, size);
	} else {
		return realloc(old, size);
	}
}

alloc_pt alloc = default_alloc; 
