#ifndef CARYLL_INCLUDE_VECTOR_H
#define CARYLL_INCLUDE_VECTOR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
void *__caryll_vector_new(size_t elem_size, __caryll_VVTI cp);

#define __caryll_VTTI_of(__typeinfo)                                                                                   \
	((__caryll_VVTI){.ctor = (void (*)(void *))((__typeinfo).ctor),                                                    \
	                 .copyctor = (void (*)(void *, const void *))((__typeinfo).copyctor),                              \
	                 .dtor = (void (*)(void *))((__typeinfo).dtor)})
#define caryll_vecInit(__ptr, __typeinfo)                                                                              \
	__caryll_vector_init((caryll_VectorVoid *)(__ptr), sizeof((__ptr)->data[0]), __caryll_VTTI_of(__typeinfo))

#define caryll_vecNew(__T, __typeinfo) __caryll_vector_new(sizeof(__T), __caryll_VTTI_of(__typeinfo));

void __caryll_vector_grow(caryll_VectorVoid *array, size_t elem_size);
#define caryll_vecPush(__ptr, __elem)                                                                                  \
	(__caryll_vector_grow((caryll_VectorVoid *)(__ptr), sizeof((__ptr)->data[0])),                                     \
	 (__ptr)->data[(__ptr)->length++] = (__elem))

void __caryll_vector_dealloc(void *array);
void __caryll_vector_clear(caryll_VectorVoid *array, size_t elem_size);
#define caryll_vecDelete(__ptr)                                                                                        \
	(__caryll_vector_clear((caryll_VectorVoid *)(__ptr), sizeof((__ptr)->data[0])), __caryll_vector_dealloc(__ptr))
#define caryll_vecReset(__ptr) (__caryll_vector_clear((caryll_VectorVoid *)(__ptr), sizeof((__ptr)->data[0])))

void __caryll_vector_replace(caryll_VectorVoid *dst, caryll_VectorVoid *src, size_t elem_size);
#define caryll_vecReplace(__dst, __src)                                                                                \
	(__caryll_vector_replace((caryll_VectorVoid *)(__dst), (caryll_VectorVoid *)(__src), sizeof((__dst)->data[0])))

/*
void __caryll_vector_filter(caryll_VectorVoid *array, size_t elem_size, bool (*fn)(void *context, void *demand),
                            void *context);
#define caryll_vecFilter(__ptr, fn, context)                                                                           \
    __caryll_vector_filter((caryll_VectorVoid *)(__ptr), sizeof((__ptr)->data[0]), fn, context)
*/

#endif
