#ifndef CARYLL_INCLUDE_VECTOR_H
#define CARYLL_INCLUDE_VECTOR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define __GET_MACRO_OTFCC_VECTOR_2(_2, _1, NAME, ...) NAME

// We assume all T have trivial move constructors.
#define caryll_VectorEntryTypeInfo(T)                                                                                  \
	struct {                                                                                                           \
		void (*ctor)(T *);                                                                                             \
		void (*copyctor)(T *, const T *);                                                                              \
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

void *__caryll_vector_alloc(size_t headersize);
void __caryll_vector_init(caryll_VectorVoid *array, size_t elem_size, __caryll_VVTI cp);

#define caryll_initVector_std(ptr)                                                                                     \
	__caryll_vector_init((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0]), ((__caryll_VVTI){NULL, NULL, NULL}))
#define caryll_initVector_dtor(ptr, typeinfo)                                                                          \
	__caryll_vector_init((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0]),                                           \
	                     ((__caryll_VVTI){.ctor = (void (*)(void *))((typeinfo).ctor),                                 \
	                                      .copyctor = (void (*)(void *, const void *))((typeinfo).copyctor),           \
	                                      .dtor = (void (*)(void *))((typeinfo).dtor)}))
#define caryll_vecInit(...)                                                                                            \
	__GET_MACRO_OTFCC_VECTOR_2(__VA_ARGS__, caryll_initVector_dtor, caryll_initVector_std)(__VA_ARGS__)

#define caryll_newVector_std(ptr) (ptr = __caryll_vector_alloc(sizeof(*ptr)), caryll_vecInit(ptr))
#define caryll_newVector_dtor(ptr, typeinfo) (ptr = __caryll_vector_alloc(sizeof(*ptr)), caryll_vecInit(ptr, typeinfo))
#define caryll_vecNew(...)                                                                                             \
	__GET_MACRO_OTFCC_VECTOR_2(__VA_ARGS__, caryll_newVector_dtor, caryll_newVector_std)(__VA_ARGS__)

void __caryll_vector_grow(caryll_VectorVoid *array, size_t elem_size);
#define caryll_vecPush(ptr, elem)                                                                                      \
	(__caryll_vector_grow((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0])), (ptr)->data[(ptr)->length++] = (elem))

void __caryll_vector_dealloc(void *array);
void __caryll_vector_clear(caryll_VectorVoid *array, size_t elem_size);
#define caryll_vecDelete(ptr)                                                                                          \
	(__caryll_vector_clear((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0])), __caryll_vector_dealloc(ptr))
#define caryll_vecReset(ptr) (__caryll_vector_clear((caryll_VectorVoid *)(ptr), sizeof((ptr)->data[0])))

void __caryll_vector_replace(caryll_VectorVoid *dst, caryll_VectorVoid *src, size_t elem_size);
#define caryll_vecReplace(dst, src)                                                                                    \
	(__caryll_vector_replace((caryll_VectorVoid *)(dst), (caryll_VectorVoid *)(src), sizeof((dst)->data[0])))

#endif
