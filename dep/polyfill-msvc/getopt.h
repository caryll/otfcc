/* Declarations for getopt.

   Copyright 2008, 2010-2012 Karl Berry.
   Copyright 1989,90,91,92,93,94,96,97,2000,05 Free Software Foundation, Inc.

   The original version of this file was part of the GNU C Library.
   Its master source is NOT part of the C library, however.
   The master source lives in libc.
   This version has been modified for use with libkpathsea.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef _GETOPT_H

#ifndef __need_getopt
#define _GETOPT_H 1
#endif

#if defined(WIN32) && !defined(__MINGW32__) && !defined(NO_KPSE_DLL)
#define KPSE_DLL 1
#endif /* WIN32 && !__MINGW32__ && !NO_KPSE_DLL */

#define KPSEDLL

#ifdef __cplusplus
extern "C" {
#endif

/* For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when `ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */

extern KPSEDLL char *optarg;

/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns -1, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

extern KPSEDLL int optind;

/* Callers store zero here to inhibit the error message `getopt' prints
   for unrecognized options.  */

extern KPSEDLL int opterr;

/* Set to an option character which was unrecognized.  */

extern KPSEDLL int optopt;

#ifndef __need_getopt
/* Describe the long-named options requested by the application.
   The LONG_OPTIONS argument to getopt_long or getopt_long_only is a vector
   of `struct option' terminated by an element containing a name which is
   zero.

   The field `has_arg' is:
   no_argument          (or 0) if the option does not take an argument,
   required_argument    (or 1) if the option requires an argument,
   optional_argument    (or 2) if the option takes an optional argument.

   If the field `flag' is not NULL, it points to a variable that is set
   to the value given in the field `val' when the option is found, but
   left unchanged if the option is not found.

   To have a long-named option do something other than set an `int' to
   a compiled-in constant, such as set a value from `optarg', set the
   option's `flag' field to zero and its `val' field to a nonzero
   value (the equivalent single-letter option character, if there is
   one).  For long options that have a zero `flag' field, `getopt'
   returns the contents of the `val' field.  */

struct option {
	const char *name;
	/* has_arg can't be an enum because some compilers complain about
	   type mismatches in all the code that assumes it is an int.  */
	int has_arg;
	int *flag;
	int val;
};

/* Names for the values of the `has_arg' field of `struct option'.  */

#define no_argument 0
#define required_argument 1
#define optional_argument 2
#endif /* need getopt */

#if defined(__GNU_LIBRARY__) || defined(WIN32) || defined(__CYGWIN__)
/* Many other libraries have conflicting prototypes for getopt, with
   differences in the consts, in stdlib.h.  To avoid compilation
   errors, only prototype getopt for the GNU C library.  And not when
   compiling with C++; g++ 4.7.0 chokes on conflicting exception
   specifications.  */
#if !defined(__cplusplus)
extern KPSEDLL int getopt(int argc, char *const *argv, const char *shortopts);
#endif
#if defined(__MINGW32__) || defined(__CYGWIN__)
#define __GETOPT_H__ /* Avoid that <unistd.h> redeclares the getopt API.  */
#endif
#elif !defined(__cplusplus)
extern KPSEDLL int getopt();
#endif

#ifndef __need_getopt
extern KPSEDLL int getopt_long(int argc, char *const *argv, const char *shortopts,
                               const struct option *longopts, int *longind);
extern KPSEDLL int getopt_long_only(int argc, char *const *argv, const char *shortopts,
                                    const struct option *longopts, int *longind);

#if defined(MAKE_KPSE_DLL) || defined(NO_KPSE_DLL) /* libkpathsea internal only */

/* Internal only.  Users should not call this directly.  */
extern int _getopt_internal(int argc, char *const *argv, const char *shortopts,
                            const struct option *longopts, int *longind, int long_only);

#endif /* MAKE_KPSE_DLL || NO_KPSE_DLL */
#endif /* need getopt */

#ifdef __cplusplus
}
#endif

/* Make sure we later can get all the definitions and declarations.  */
#undef __need_getopt

#endif /* _GETOPT_H */
