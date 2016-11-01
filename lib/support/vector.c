#include "caryll/vector.h"
#include <stdio.h>

void __caryll_vector_init(caryll_VectorVoid *array, size_t elem_size, __caryll_VVTI cp) {
	array->length = 0;
	array->capacity = 0;
	array->iElement.ctor = cp.ctor;
	array->iElement.dtor = cp.dtor;
	array->iElement.copyctor = cp.copyctor;
	array->iElement.movector = cp.movector;
	array->data = NULL;
}
void __caryll_vector_grow(caryll_VectorVoid *array, size_t elem_size) {
	if (array->length < array->capacity) return;
	if (!array->capacity) array->capacity = 0x10;
	array->capacity += array->capacity / 2;
	if (array->data) {
		void *newdata = calloc(array->capacity, elem_size);
		if (array->iElement.movector) {
			for (size_t j = 0; j < array->length; j++) {
				array->iElement.movector((void *)((char *)(newdata) + elem_size * j),
				                         (void *)((char *)(array->data) + elem_size * j));
			}
			free(array->data);
		} else if (array->iElement.copyctor) {
			for (size_t j = 0; j < array->length; j++) {
				array->iElement.copyctor((void *)((char *)(newdata) + elem_size * j),
				                         (void *)((char *)(array->data) + elem_size * j));
			}
			if (array->iElement.dtor) {
				for (size_t j = array->length; j--;) {
					array->iElement.dtor((void *)((char *)(array->data) + elem_size * j));
				}
			}
			free(array->data);
		} else {
			memcpy(newdata, array->data, array->length * elem_size);
			free(array->data);
		}
		array->data = newdata;
	} else {
		array->data = calloc(array->capacity, elem_size);
	}
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
