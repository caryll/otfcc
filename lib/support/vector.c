#include "caryll/vector.h"
#include "support/util.h"
#include <stdio.h>

void *__caryll_vector_alloc(size_t headersize) {
	return __otfcc_malloc(headersize);
}

void __caryll_vector_dealloc(void *array) {
	__otfcc_free(array);
}

void __caryll_vector_init(caryll_VectorVoid *array, size_t elem_size, __caryll_VVTI cp) {
	array->length = 0;
	array->capacity = 0;
	array->iElement.ctor = cp.ctor;
	array->iElement.dtor = cp.dtor;
	array->iElement.copyctor = cp.copyctor;
	array->data = NULL;
}

void *__caryll_vector_new(size_t elem_size, __caryll_VVTI cp) {
	caryll_VectorVoid *ptr = __caryll_vector_alloc(sizeof(*ptr));
	__caryll_vector_init(ptr, elem_size, cp);
	return ptr;
}

void __caryll_vector_grow(caryll_VectorVoid *array, size_t elem_size) {
	if (array->length < array->capacity) return;
	if (!array->capacity) array->capacity = 0x10;
	array->capacity += array->capacity / 2;
	if (array->data) {
		array->data = realloc(array->data, array->capacity * elem_size);
	} else {
		array->data = calloc(array->capacity, elem_size);
	}
}

void __caryll_vector_pop(caryll_VectorVoid *array, size_t elem_size) {
	if (array->iElement.dtor) {
		array->iElement.dtor((void *)((char *)(array->data) + elem_size * (array->length - 1)));
	}
	array->length -= 1;
}

void __caryll_vector_clear(caryll_VectorVoid *array, size_t elem_size) {
	if (array->iElement.dtor) {
		for (size_t j = array->length; j--;) {
			array->iElement.dtor((void *)((char *)(array->data) + elem_size * j));
		}
	}
	free(array->data);
	array->data = NULL;
	array->length = 0;
	array->capacity = 0;
}

void __caryll_vector_replace(caryll_VectorVoid *dst, caryll_VectorVoid *src, size_t elem_size) {
	__caryll_vector_clear(dst, elem_size);
	memcpy(dst, src, sizeof(caryll_VectorVoid));
}
