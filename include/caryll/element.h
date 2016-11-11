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

#define __NARG__(...) __NARG_I_(__VA_ARGS__, __RSEQ_N())
#define __NARG_I_(...) __ARG_N(__VA_ARGS__)
#define __ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22,   \
                _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42,    \
                _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62,    \
                _63, N, ...)                                                                                           \
	N
#define __RSEQ_N()                                                                                                     \
	63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36,    \
	    35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8,  \
	    7, 6, 5, 4, 3, 2, 1, 0

// general definition for any function name
#define __CARYLL_VFUNC__2(name, n) name##n
#define __CARYLL_VFUNC__1(name, n) __CARYLL_VFUNC__2(name, n)
#define __CARYLLVFUNC(func, ...) __CARYLL_VFUNC__1(func, __NARG__(__VA_ARGS__))(__VA_ARGS__)

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

#define caryll_trivialInit(T)                                                                                          \
	static __CARYLL_INLINE__ void T##_init(MODIFY T *x) {                                                              \
		memset(x, 0, sizeof(T));                                                                                       \
	}
#define caryll_nonTrivialInit(T, ctor)                                                                                 \
	static __CARYLL_INLINE__ void T##_init(MODIFY T *x) {                                                              \
		(ctor)(x);                                                                                                     \
	}
#define caryll_trivialEmpty(T)                                                                                         \
	static __CARYLL_INLINE__ T T##_empty() {                                                                           \
		T x;                                                                                                           \
		T##_init(&x);                                                                                                  \
		return x;                                                                                                      \
	}
#define caryll_trivialCreate(T)                                                                                        \
	static __CARYLL_INLINE__ T *T##_create() {                                                                         \
		T *x = (T *)malloc(sizeof(T));                                                                                 \
		T##_init(x);                                                                                                   \
		return x;                                                                                                      \
	}
#define caryll_trivialCopy(T)                                                                                          \
	static __CARYLL_INLINE__ void T##_copy(MODIFY T *dst, const T *src) {                                              \
		memcpy(dst, src, sizeof(T));                                                                                   \
	}
#define caryll_nonTrivialCopy(T, cp)                                                                                   \
	static __CARYLL_INLINE__ void T##_copy(MODIFY T *dst, const T *src) {                                              \
		(cp)(dst, src);                                                                                                \
	}
#define caryll_trivialDispose(T)                                                                                       \
	static __CARYLL_INLINE__ void T##_dispose(MODIFY T *x) {}
#define caryll_nonTrivialDispose(T, fn)                                                                                \
	static __CARYLL_INLINE__ void T##_dispose(MODIFY T *x) {                                                           \
		(fn)(x);                                                                                                       \
	}
#define caryll_trivialDestroy(T)                                                                                       \
	static __CARYLL_INLINE__ void T##_destroy(MOVE T *x) {                                                             \
		if (!x) return;                                                                                                \
		T##_dispose(x);                                                                                                \
		__caryll_free(x);                                                                                              \
	}
#define caryll_trivialDup(T)                                                                                           \
	static __CARYLL_INLINE__ T T##_dup(const T src) {                                                                  \
		T dst;                                                                                                         \
		T##_copy(&dst, &src);                                                                                          \
		return dst;                                                                                                    \
	}

#define caryll_standardTypeFn1(T)                                                                                      \
	caryll_trivialInit(T);                                                                                             \
	caryll_trivialCopy(T);                                                                                             \
	caryll_trivialDispose(T);
#define caryll_standardTypeFn2(T, __fn_dispose)                                                                        \
	caryll_trivialInit(T);                                                                                             \
	caryll_trivialCopy(T);                                                                                             \
	caryll_nonTrivialDispose(T, __fn_dispose);
#define caryll_standardTypeFn3(T, __fn_init, __fn_dispose)                                                             \
	caryll_nonTrivialInit(T, __fn_init);                                                                               \
	caryll_trivialCopy(T);                                                                                             \
	caryll_nonTrivialDispose(T, __fn_dispose);
#define caryll_standardTypeFn4(T, __fn_init, __fn_copy, __fn_dispose)                                                  \
	caryll_nonTrivialInit(T, __fn_init);                                                                               \
	caryll_nonTrivialCopy(T, __fn_copy);                                                                               \
	caryll_nonTrivialDispose(T, __fn_dispose);

#define caryll_standardTypeFn(...) __CARYLLVFUNC(caryll_standardTypeFn, __VA_ARGS__)
#define caryll_standardRefTypeFn(T, ...)                                                                               \
	caryll_standardTypeFn(T, __VA_ARGS__);                                                                             \
	caryll_trivialCreate(T);                                                                                           \
	caryll_trivialDestroy(T);
#define caryll_standardValTypeFn(T, ...)                                                                               \
	caryll_standardTypeFn(T, __VA_ARGS__);                                                                             \
	caryll_trivialEmpty(T);                                                                                            \
	caryll_trivialDup(T);

#define caryll_DefaultTAsg(T) .init = T##_init, .copy = T##_copy, .dispose = T##_dispose
#define caryll_DefaultRTAsg(T) caryll_DefaultTAsg(T), .create = T##_create, .destroy = T##_destroy
#define caryll_DefaultVTAsg(T) caryll_DefaultTAsg(T), .empty = T##_empty, .dup = T##_dup

#define caryll_standardType(T, __name, ...)                                                                            \
	caryll_standardTypeFn(T, __VA_ARGS__);                                                                             \
	caryll_ElementInterfaceOf(T) __name = {                                                                            \
	    caryll_DefaultTAsg(T),                                                                                         \
	};
#define caryll_standardRefType(T, __name, ...)                                                                         \
	caryll_standardRefTypeFn(T, __VA_ARGS__);                                                                          \
	caryll_ElementInterfaceOf(T) __name = {                                                                            \
	    caryll_DefaultRTAsg(T),                                                                                        \
	};
#define caryll_standardValType(T, __name, ...)                                                                         \
	caryll_standardValTypeFn(T, __VA_ARGS__);                                                                          \
	caryll_ElementInterfaceOf(T) __name = {                                                                            \
	    caryll_DefaultVTAsg(T),                                                                                        \
	};

#endif
