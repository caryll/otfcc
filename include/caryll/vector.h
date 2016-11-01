#ifndef CARYLL_INCLUDE_VECTOR_H
#define CARYLL_INCLUDE_VECTOR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define __GET_MACRO_OTFCC_VECTOR_2(_2, _1, NAME, ...) NAME

#define caryll_VectorEntryTypeInfo(T)                                                                                  \
	struct {                                                                                                           \
		void (*ctor)(T *);                                                                                             \
		void (*copyctor)(T *, const T *);                                                                              \
		void (*movector)(T *, T *);                                                                                    \
		void (*dtor)(T *);                                                                                             \
	}

#define caryll_Vector(T)                                                                                               \
	struct {                                                                                                           \
		size_t length;                                                                                                 \
		size_t capacity;                                                                                               \
		caryll_VectorEntryTypeInfo(T) iElement;                                                                        \
		T *data;                                                                                                       \
	}

typedef caryll_Vector(void) caryll_VectorVoid;
typedef caryll_VectorEntryTypeInfo(void) __caryll_VVTI;

void __caryll_vector_init(caryll_VectorVoid *array, size_t elem_size, __caryll_VVTI cp);

#define caryll_initVector_std(ptr)                                                                                     \
	__caryll_vector_init((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0]), ((__caryll_VVTI){NULL, NULL, NULL, NULL}))
#define caryll_initVector_dtor(ptr, typeinfo)                                                                          \
	__caryll_vector_init((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0]),                                           \
	                     ((__caryll_VVTI){.ctor = (void (*)(void *))((typeinfo).ctor),                                 \
	                                      .copyctor = (void (*)(void *, const void *))((typeinfo).copyctor),           \
	                                      .movector = (void (*)(void *, void *))((typeinfo).movector),                 \
	                                      .dtor = (void (*)(void *))((typeinfo).dtor)}))
#define caryll_initVector(...)                                                                                         \
	__GET_MACRO_OTFCC_VECTOR_2(__VA_ARGS__, caryll_initVector_dtor, caryll_initVector_std)(__VA_ARGS__)

#define caryll_newVector_std(ptr) (ptr = malloc(sizeof(*ptr)), caryll_initVector(ptr))
#define caryll_newVector_dtor(ptr, typeinfo) (ptr = malloc(sizeof(*ptr)), caryll_initVector(ptr, typeinfo))
#define caryll_newVector(...)                                                                                          \
	__GET_MACRO_OTFCC_VECTOR_2(__VA_ARGS__, caryll_newVector_dtor, caryll_newVector_std)(__VA_ARGS__)

void __caryll_vector_grow(caryll_VectorVoid *array, size_t elem_size);
#define caryll_pushVector(ptr, elem)                                                                                   \
	(__caryll_vector_grow((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0])), (ptr)->data[(ptr)->length++] = (elem))

void __caryll_vector_clear(caryll_VectorVoid *array, size_t elem_size);
#define caryll_deleteVector(ptr) (__caryll_vector_clear((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0])), free(ptr))
#define caryll_resetVector(ptr) (__caryll_vector_clear((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0])))

#endif
