#ifndef CARYLL_INCLUDE_ELEMENT_H
#define CARYLL_INCLUDE_ELEMENT_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// We assume all T have trivial move constructors.
#define caryll_T(T)                                                                                                    \
	void (*init)(MODIFY T *);                                                                                          \
	void (*copy)(MODIFY T *, const T *);                                                                               \
	void (*move)(MODIFY T *, T *);                                                                                     \
	void (*dispose)(MOVE T *);
#define caryll_VT(T)                                                                                                   \
	caryll_T(T);                                                                                                       \
	T (*empty)();                                                                                                      \
	T (*dup)(const T);
#define caryll_RT(T)                                                                                                   \
	caryll_T(T);                                                                                                       \
	T *(*create)();                                                                                                    \
	void (*free)(MOVE T *);

#define caryll_ElementInterfaceOf(T) const struct __caryll_elementinterface_##T
#define caryll_ElementInterface(T)                                                                                     \
	caryll_ElementInterfaceOf(T) {                                                                                     \
		caryll_T(T);                                                                                                   \
	}
#define caryll_RefElementInterface(T)                                                                                  \
	caryll_ElementInterfaceOf(T) {                                                                                     \
		caryll_RT(T);                                                                                                  \
	}
#define caryll_ValElementInterface(T)                                                                                  \
	caryll_ElementInterfaceOf(T) {                                                                                     \
		caryll_VT(T);                                                                                                  \
	}

#endif
