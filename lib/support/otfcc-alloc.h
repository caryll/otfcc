#ifndef CARYLL_SUPPORT_OTFCC_ALLOC_H
#define CARYLL_SUPPORT_OTFCC_ALLOC_H

#include <stdio.h>
#include <stdlib.h>

#ifndef INLINE
#ifdef _MSC_VER
#define INLINE __forceinline /* use __forceinline (VC++ specific) */
#else
#define INLINE inline /* use standard inline */
#endif
#endif

// Allocators
// Change this if you prefer other allocators
#define __otfcc_malloc malloc
#define __otfcc_calloc calloc
#define __otfcc_realloc realloc
#define __otfcc_free free

static INLINE void *__caryll_allocate_dirty(size_t n, unsigned long line) {
	if (!n) return NULL;
	void *p = __otfcc_malloc(n);
	if (!p) {
		fprintf(stderr, "[%ld]Out of memory(%ld bytes)\n", line, (unsigned long)n);
		exit(EXIT_FAILURE);
	}
	return p;
}
static INLINE void *__caryll_allocate_clean(size_t n, unsigned long line) {
	if (!n) return NULL;
	void *p = __otfcc_calloc(n, 1);
	if (!p) {
		fprintf(stderr, "[%ld]Out of memory(%ld bytes)\n", line, (unsigned long)n);
		exit(EXIT_FAILURE);
	}
	return p;
}
static INLINE void *__caryll_reallocate(void *ptr, size_t n, unsigned long line) {
	if (!n) {
		__otfcc_free(ptr);
		return NULL;
	}
	if (!ptr) {
		return __caryll_allocate_clean(n, line);
	} else {
		void *p = __otfcc_realloc(ptr, n);
		if (!p) {
			fprintf(stderr, "[%ld]Out of memory(%ld bytes)\n", line, (unsigned long)n);
			exit(EXIT_FAILURE);
		}
		return p;
	}
}
#ifdef __cplusplus
#define NEW(ptr) ptr = (decltype(ptr))__caryll_allocate_clean(sizeof(decltype(*ptr)), __LINE__)
#define NEW_DIRTY(ptr) ptr = (decltype(ptr))__caryll_allocate_dirty(sizeof(decltype(*ptr)), __LINE__)
#define NEW_CLEAN(ptr) ptr = (decltype(ptr))__caryll_allocate_clean(sizeof(decltype(*ptr)), __LINE__)
#define NEW_N(ptr, n) ptr = (decltype(ptr))__caryll_allocate_clean(sizeof(decltype(*ptr)) * (n), __LINE__)
#define FREE(ptr) (__otfcc_free(ptr), ptr = nullptr)
#define DELETE(fn, ptr) (fn(ptr), ptr = nullptr)
#define RESIZE(ptr, n) ptr = (decltype(ptr))__caryll_reallocate(ptr, sizeof(*ptr) * (n), __LINE__)
#else
#define NEW(ptr) ptr = __caryll_allocate_clean(sizeof(*ptr), __LINE__)
#define NEW_DIRTY(ptr) ptr = __caryll_allocate_dirty(sizeof(*ptr), __LINE__)
#define NEW_CLEAN(ptr) ptr = __caryll_allocate_clean(sizeof(*ptr), __LINE__)
#define NEW_N(ptr, n) ptr = __caryll_allocate_clean(sizeof(*ptr) * (n), __LINE__)
#define FREE(ptr) (__otfcc_free(ptr), ptr = NULL)
#define DELETE(fn, ptr) (fn(ptr), ptr = NULL)
#define RESIZE(ptr, n) ptr = __caryll_reallocate(ptr, sizeof(*ptr) * (n), __LINE__)
#endif

#endif
