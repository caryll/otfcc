#ifndef CARYLL_INCLUDE_VECTOR_H
#define CARYLL_INCLUDE_VECTOR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "element.h"

#ifndef __CARYLL_INLINE__
#ifdef _MSC_VER
#define __CARYLL_INLINE__ __forceinline /* use __forceinline (VC++ specific) */
#else
#define __CARYLL_INLINE__ inline /* use standard inline */
#endif
#endif

#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define __CARYLL_MAY_UNUSED__ __attribute__((__unused__))
#elif (__clang_major__ > 2)
#define __CARYLL_MAY_UNUSED__ __attribute__((__unused__))
#else
#define __CARYLL_MAY_UNUSED__
#endif

// We assume all T have trivial move constructors.
#define caryll_Vector(T)                                                                                               \
	struct {                                                                                                           \
		size_t length;                                                                                                 \
		size_t capacity;                                                                                               \
		T *items;                                                                                                      \
	}
#define caryll_VectorInterfaceTypeName(__TV) const struct __caryll_vectorinterface_##__TV
#define caryll_VectorInterfaceTrait(__TV, __T)                                                                         \
	caryll_RT(__TV);                                                                                                   \
	void (*initN)(MODIFY __TV * arr, size_t n);                                                                        \
	__TV *(*createN)(size_t n);                                                                                        \
	void (*clear)(MODIFY __TV * arr);                                                                                  \
	void (*replace)(MODIFY __TV * dst, MOVE const __TV *src);                                                          \
	void (*push)(MODIFY __TV * arr, MOVE __T obj);                                                                     \
	__T (*pop)(MODIFY __TV * arr);                                                                                     \
	void (*fill)(MODIFY __TV * arr, size_t n);                                                                         \
	void (*disposeItem)(MODIFY __TV * arr, size_t n);                                                                  \
	void (*sort)(MODIFY __TV * arr, int (*fn)(const __T *a, const __T *b));

#define caryll_VectorInterface(__TV, __T)                                                                              \
	caryll_VectorInterfaceTypeName(__TV) {                                                                             \
		caryll_VectorInterfaceTrait(__TV, __T);                                                                        \
	}

#define caryll_VectorImplDestroyIndependent(__TV, __T, __ti, __name)                                                   \
	static __CARYLL_INLINE__ void __name##_dispose(__TV *arr) {                                                        \
		if (!arr) return;                                                                                              \
		if (__ti.dispose) {                                                                                            \
			for (size_t j = arr->length; j--;) {                                                                       \
				__ti.dispose(&arr->items[j]);                                                                          \
			}                                                                                                          \
		}                                                                                                              \
		__caryll_free(arr->items);                                                                                     \
		arr->items = NULL;                                                                                             \
		arr->length = 0;                                                                                               \
		arr->capacity = 0;                                                                                             \
	}

#define caryll_VectorImplDestroyDependent(__TV, __T, __ti, __name)                                                     \
	static __CARYLL_INLINE__ void __name##_dispose(__TV *arr) {                                                        \
		if (!arr) return;                                                                                              \
		if (__ti.disposeDependent) {                                                                                   \
			for (size_t j = arr->length; j--;) {                                                                       \
				__ti.disposeDependent(&arr->items[j], arr);                                                            \
			}                                                                                                          \
		}                                                                                                              \
		__caryll_free(arr->items);                                                                                     \
		arr->items = NULL;                                                                                             \
		arr->length = 0;                                                                                               \
		arr->capacity = 0;                                                                                             \
	}

#define caryll_VectorImplFunctionsCommon(__TV, __T, __ti, __name)                                                      \
	static __CARYLL_INLINE__ void __name##_init(MODIFY __TV *arr) {                                                    \
		arr->length = 0;                                                                                               \
		arr->capacity = 0;                                                                                             \
		arr->items = NULL;                                                                                             \
	}                                                                                                                  \
	static __CARYLL_INLINE__ __TV *__name##_create() {                                                                 \
		__TV *t = __caryll_malloc(sizeof(__TV));                                                                       \
		__name##_init(t);                                                                                              \
		return t;                                                                                                      \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_destroy(MOVE __TV *arr) {                                                   \
		if (!arr) return;                                                                                              \
		__name##_dispose(arr);                                                                                         \
		__caryll_free(arr);                                                                                            \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_growTo(MODIFY __TV *arr, size_t target) {                                   \
		if (target <= arr->capacity) return;                                                                           \
		if (!arr->capacity) arr->capacity = 0x10;                                                                      \
		while (arr->capacity <= target) {                                                                              \
			arr->capacity += arr->capacity / 2;                                                                        \
		}                                                                                                              \
		if (arr->items) {                                                                                              \
			arr->items = __caryll_realloc(arr->items, arr->capacity * sizeof(__T));                                    \
		} else {                                                                                                       \
			arr->items = __caryll_calloc(arr->capacity, sizeof(__T));                                                  \
		}                                                                                                              \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_grow(MODIFY __TV *arr) {                                                    \
		__name##_growTo(arr, arr->length + 1);                                                                         \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_push(MODIFY __TV *arr, MOVE __T elem) {                                     \
		__name##_grow(arr);                                                                                            \
		(arr)->items[(arr)->length++] = (elem);                                                                        \
	}                                                                                                                  \
	static __CARYLL_INLINE__ __T __name##_pop(MODIFY __TV *arr) {                                                      \
		__T t = arr->items[arr->length - 1];                                                                           \
		arr->length -= 1;                                                                                              \
		return t;                                                                                                      \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_fill(MODIFY __TV *arr, size_t n) {                                          \
		while (arr->length < n) {                                                                                      \
			__T x;                                                                                                     \
			if (__ti.init) {                                                                                           \
				__ti.init(&x);                                                                                         \
			} else {                                                                                                   \
				memset(&x, 0, sizeof(x));                                                                              \
			}                                                                                                          \
			__name##_push(arr, x);                                                                                     \
		}                                                                                                              \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_initN(MODIFY __TV *arr, size_t n) {                                         \
		__name##_init(arr);                                                                                            \
		__name##_fill(arr, n);                                                                                         \
	}                                                                                                                  \
	static __CARYLL_INLINE__ __TV *__name##_createN(size_t n) {                                                        \
		__TV *t = __caryll_malloc(sizeof(__TV));                                                                       \
		__name##_initN(t, n);                                                                                          \
		return t;                                                                                                      \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_copy(MODIFY __TV *dst, const __TV *src) {                                   \
		__name##_init(dst);                                                                                            \
		__name##_growTo(dst, src->length);                                                                             \
		if (__ti.copy) {                                                                                               \
			for (size_t j = 0; j < src->length; j++) {                                                                 \
				__ti.copy(&dst->items[j], (const __T *)&src->items[j]);                                                \
			}                                                                                                          \
		} else {                                                                                                       \
			for (size_t j = 0; j < src->length; j++) {                                                                 \
				dst->items[j] = src->items[j];                                                                         \
			}                                                                                                          \
		}                                                                                                              \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_replace(MODIFY __TV *dst, MOVE const __TV *src) {                           \
		__name##_dispose(dst);                                                                                         \
		memcpy(dst, src, sizeof(__TV));                                                                                \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_sort(MODIFY __TV *arr, int (*fn)(const __T *a, const __T *b)) {             \
		qsort(arr->items, arr->length, sizeof(arr->items[0]), (int (*)(const void *, const void *))fn);                \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void __name##_disposeItem(MODIFY __TV *arr, size_t n) {                                   \
		__ti.dispose ? __ti.dispose(&((arr)->items[n])) : (void)0;                                                     \
	}

#define caryll_VectorImplFunctions(__TV, __T, __ti, __name)                                                            \
	caryll_VectorImplDestroyIndependent(__TV, __T, __ti, __name);                                                      \
	caryll_VectorImplFunctionsCommon(__TV, __T, __ti, __name);

#define caryll_VectorImplAssignments(__TV, __T, __ti, __name)                                                          \
	.init = __name##_init, .copy = __name##_copy, .dispose = __name##_dispose, .create = __name##_create,              \
	.createN = __name##_createN, .destroy = __name##_destroy, .initN = __name##_initN, .clear = __name##_dispose,      \
	.replace = __name##_replace, .push = __name##_push, .pop = __name##_pop, .fill = __name##_fill,                    \
	.sort = __name##_sort, .disposeItem = __name##_disposeItem

#define caryll_DefineVectorImpl(__TV, __T, __ti, __name)                                                               \
	caryll_VectorImplFunctions(__TV, __T, __ti, __name);                                                               \
	caryll_VectorInterfaceTypeName(__TV) __name = {                                                                    \
	    caryll_VectorImplAssignments(__TV, __T, __ti, __name),                                                         \
	};

#endif
