#ifndef CARYLL_INCLUDE_ELEMENT_H
#define CARYLL_INCLUDE_ELEMENT_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef __CARYLL_INLINE__
#ifdef _MSC_VER
#define __CARYLL_INLINE__ __forceinline /* use __forceinline (VC++ specific) */
#else
#define __CARYLL_INLINE__ inline /* use standard inline */
#endif
#endif

#define __caryll_malloc malloc
#define __caryll_calloc calloc
#define __caryll_realloc realloc
#define __caryll_free free

// We assume all T have trivial move constructors.
#define caryll_T(T)                                                                                                    \
	void (*init)(MODIFY T *);                                                                                          \
	void (*copy)(MODIFY T *, const T *);                                                                               \
	void (*dispose)(MOVE T *);
#define caryll_VT(T)                                                                                                   \
	caryll_T(T);                                                                                                       \
	T (*empty)();                                                                                                      \
	T (*dup)(const T);
#define caryll_RT(T)                                                                                                   \
	caryll_T(T);                                                                                                       \
	T *(*create)();                                                                                                    \
	void (*destroy)(MOVE T *);

#define caryll_ElementInterfaceOf(T) const struct __caryll_elementinterface_##T
#define caryll_ElementInterface(T)                                                                                     \
	caryll_ElementInterfaceOf(T) {                                                                                     \
		caryll_T(T);                                                                                                   \
	}
#define caryll_RefElementInterface(T)                                                                                  \
	caryll_ElementInterfaceOf(T) {                                                                                     \
		caryll_RT(T);                                                                                                  \
	}

#define caryll_TrivialElementImpl(T, name)                                                                             \
	static __CARYLL_INLINE__ void name##_init(MODIFY T *x) {                                                           \
		memset(x, 0, sizeof(T));                                                                                       \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void name##_copy(MODIFY T *dst, const T *src) {                                           \
		memcpy(dst, src, sizeof(T));                                                                                   \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void name##_dispose(MODIFY T *x) {}                                                       \
	caryll_ElementInterfaceOf(T) name = {                                                                              \
	    .init = name##_init, .copy = name##_copy, .dispose = name##_dispose,                                           \
	}

#define caryll_DtorElementImpl(T, dtor, name)                                                                          \
	static __CARYLL_INLINE__ void name##_init(MODIFY T *x) {                                                           \
		memset(x, 0, sizeof(T));                                                                                       \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void name##_copy(MODIFY T *dst, const T *src) {                                           \
		memcpy(dst, src, sizeof(T));                                                                                   \
	}                                                                                                                  \
	caryll_ElementInterfaceOf(T) name = {                                                                              \
	    .init = name##_init, .copy = name##_copy, .dispose = dtor,                                                     \
	}

#define caryll_CDRefElementImplFn(T, ctor, dtor, name)                                                                 \
	static __CARYLL_INLINE__ T *name##_create() {                                                                      \
		T *x = (T *)malloc(sizeof(T));                                                                                 \
		ctor(x);                                                                                                       \
		return x;                                                                                                      \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void name##_copy(MODIFY T *dst, const T *src) {                                           \
		memcpy(dst, src, sizeof(T));                                                                                   \
	}                                                                                                                  \
	static __CARYLL_INLINE__ void name##_destroy(MOVE T *x) {                                                          \
		if (!x) return;                                                                                                \
		dtor(x);                                                                                                       \
		__caryll_free(x);                                                                                              \
	}

#define caryll_CdRefElementImplAsg(T, ctor, dtor, name)                                                                \
	.init = ctor, .copy = name##_copy, .dispose = dtor, .create = name##_create, .destroy = name##_destroy

#define caryll_CDRefElementImpl(T, ctor, dtor, name)                                                                   \
	caryll_CDRefElementImplFn(T, ctor, dtor, name);                                                                    \
	caryll_ElementInterfaceOf(T) name = {                                                                              \
	    caryll_CdRefElementImplAsg(T, ctor, dtor, name),                                                               \
	}

#endif
