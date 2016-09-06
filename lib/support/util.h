#ifndef CARYLL_SUPPORT_UTIL_H
#define CARYLL_SUPPORT_UTIL_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "base64.h"
#include "buffer.h"
#include "options.h"
#include "json-ident.h"
#include <extern/json-builder.h>
#include <extern/json.h>
#include <extern/sds.h>
#include <extern/uthash.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define INLINE __forceinline /* use __forceinline (VC++ specific) */
#else
#define INLINE inline /* use standard inline */
#endif

#define FOR_TABLE(name, table)                                                                                         \
	for (int keep = 1, count = 0, __notfound = 1; __notfound && keep && count < packet.numTables;                      \
	     keep = !keep, count++)                                                                                        \
		for (caryll_PacketPiece table = (packet.pieces)[count]; keep; keep = !keep)                                          \
			if (table.tag == (name))                                                                                   \
				for (int k2 = 1; k2; k2 = 0, __notfound = 0)

#define foreach_hash(id, range) for (id = (range); id != NULL; id = id->hh.next)

static INLINE json_value *json_obj_get(json_value *obj, const char *key) {
	if (!obj || obj->type != json_object) return NULL;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		if (strcmp(ck, key) == 0) return obj->u.object.values[_k].value;
	}
	return NULL;
}
static INLINE json_value *json_obj_get_type(json_value *obj, const char *key, json_type type) {
	json_value *v = json_obj_get(obj, key);
	if (v && v->type == type) return v;
	return NULL;
}
static INLINE sds json_obj_getsds(json_value *obj, const char *key) {
	json_value *v = json_obj_get_type(obj, key, json_string);
	if (!v)
		return NULL;
	else
		return sdsnewlen(v->u.string.ptr, v->u.string.length);
}
static INLINE char *json_obj_getstr_share(json_value *obj, const char *key) {
	json_value *v = json_obj_get_type(obj, key, json_string);
	if (!v)
		return NULL;
	else
		return v->u.string.ptr;
}
static INLINE double json_numof(json_value *cv) {
	if (cv && cv->type == json_integer) return cv->u.integer;
	if (cv && cv->type == json_double) return cv->u.dbl;
	return 0;
}
static INLINE double json_obj_getnum(json_value *obj, const char *key) {
	if (!obj || obj->type != json_object) return 0.0;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_integer) return cv->u.integer;
			if (cv && cv->type == json_double) return cv->u.dbl;
		}
	}
	return 0.0;
}
static INLINE int32_t json_obj_getint(json_value *obj, const char *key) {
	if (!obj || obj->type != json_object) return 0;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_integer) return (int32_t)cv->u.integer;
			if (cv && cv->type == json_double) return cv->u.dbl;
		}
	}
	return 0;
}
static INLINE double json_obj_getnum_fallback(json_value *obj, const char *key, double fallback) {
	if (!obj || obj->type != json_object) return fallback;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_integer) return cv->u.integer;
			if (cv && cv->type == json_double) return cv->u.dbl;
		}
	}
	return fallback;
}
static INLINE int32_t json_obj_getint_fallback(json_value *obj, const char *key, int32_t fallback) {
	if (!obj || obj->type != json_object) return fallback;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_integer) return (int32_t)cv->u.integer;
			if (cv && cv->type == json_double) return cv->u.dbl;
		}
	}
	return fallback;
}
static INLINE bool json_boolof(json_value *cv) {
	if (cv && cv->type == json_boolean) return cv->u.boolean;
	return false;
}
static INLINE bool json_obj_getbool(json_value *obj, const char *key) {
	if (!obj || obj->type != json_object) return false;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_boolean) return cv->u.boolean;
		}
	}
	return false;
}
static INLINE bool json_obj_getbool_fallback(json_value *obj, const char *key, bool fallback) {
	if (!obj || obj->type != json_object) return fallback;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_boolean) return cv->u.boolean;
		}
	}
	return fallback;
}

static INLINE json_value *json_from_sds(sds str) {
	return json_string_new_length((uint32_t)sdslen(str), str);
}

// flags reader and writer
static INLINE json_value *caryll_dump_flags(int flags, const char *labels[]) {
	json_value *v = json_object_new(0);
	for (uint16_t j = 0; labels[j]; j++)
		if (flags & (1 << j)) { json_object_push(v, labels[j], json_boolean_new(true)); }
	return v;
}
static INLINE uint32_t caryll_parse_flags(json_value *v, const char *labels[]) {
	if (!v) return 0;
	if (v->type == json_integer) {
		return (uint32_t)v->u.integer;
	} else if (v->type == json_double) {
		return (uint32_t)v->u.dbl;
	} else if (v->type == json_object) {
		uint32_t flags = 0;
		for (uint16_t j = 0; labels[j]; j++) {
			if (json_obj_getbool(v, labels[j])) { flags |= (1 << j); }
		}
		return flags;
	} else {
		return 0;
	}
}

// inline io util functions
typedef uint8_t *font_file_pointer;

static INLINE bool caryll_check_endian(void) {
	union {
		uint8_t i1[2];
		uint16_t i2;
	} check_union = {.i2 = 1}; // if you don't have a new VC, upgrade it

	return (check_union.i1[0] == 1);
}

static INLINE uint16_t caryll_endian_convert16(uint16_t i) {
	if (caryll_check_endian()) {
		union {
			uint8_t i1[2];
			uint16_t i2;
		} src, des;

		src.i2 = i;

		des.i1[0] = src.i1[1];
		des.i1[1] = src.i1[0];

		return des.i2;
	} else {
		return i;
	}
}

static INLINE uint32_t caryll_endian_convert32(uint32_t i) {
	if (caryll_check_endian()) {
		union {
			uint8_t i1[4];
			uint32_t i4;
		} src, des;

		src.i4 = i;

		des.i1[0] = src.i1[3];
		des.i1[1] = src.i1[2];
		des.i1[2] = src.i1[1];
		des.i1[3] = src.i1[0];

		return des.i4;
	} else {
		return i;
	}
}

static INLINE uint64_t caryll_endian_convert64(uint64_t i) {
	if (caryll_check_endian()) {
		union {
			uint8_t i1[8];
			uint64_t i8;
		} src, des;

		src.i8 = i;

		des.i1[0] = src.i1[7];
		des.i1[1] = src.i1[6];
		des.i1[2] = src.i1[5];
		des.i1[3] = src.i1[4];
		des.i1[4] = src.i1[3];
		des.i1[5] = src.i1[2];
		des.i1[6] = src.i1[1];
		des.i1[7] = src.i1[0];

		return des.i8;
	} else {
		return i;
	}
}

static INLINE uint16_t caryll_get16u(FILE *file) {
	uint16_t tmp;
	fread(&tmp, 2, 1, file);
	return caryll_endian_convert16(tmp);
}

static INLINE uint32_t caryll_get32u(FILE *file) {
	uint32_t tmp;
	fread(&tmp, 4, 1, file);
	return caryll_endian_convert32(tmp);
}

static INLINE uint64_t caryll_get64u(FILE *file) {
	uint64_t tmp;
	fread(&tmp, 8, 1, file);
	return caryll_endian_convert64(tmp);
}

// data reader
static INLINE uint8_t read_8u(uint8_t *src) {
	return src[0];
}
static INLINE uint16_t read_16u(uint8_t *src) {
	uint16_t b0 = ((uint16_t)src[0]) << 8;
	uint16_t b1 = ((uint16_t)src[1]);
	return (b0 | b1);
}
static INLINE uint32_t read_32u(uint8_t *src) {
	uint32_t b0 = ((uint32_t)src[0]) << 24;
	uint32_t b1 = ((uint32_t)src[1]) << 16;
	uint32_t b2 = ((uint32_t)src[2]) << 8;
	uint32_t b3 = ((uint32_t)src[3]);
	return (b0 | b1 | b2 | b3);
}
static INLINE uint64_t read_64u(uint8_t *src) {
	uint64_t b0 = ((uint64_t)src[0]) << 56;
	uint64_t b1 = ((uint64_t)src[1]) << 48;
	uint64_t b2 = ((uint64_t)src[2]) << 40;
	uint64_t b3 = ((uint64_t)src[3]) << 32;
	uint64_t b4 = ((uint64_t)src[4]) << 24;
	uint64_t b5 = ((uint64_t)src[5]) << 16;
	uint64_t b6 = ((uint64_t)src[6]) << 8;
	uint64_t b7 = ((uint64_t)src[7]);
	return (b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7);
}
static INLINE int8_t read_8s(uint8_t *src) {
	return (int8_t)read_8u(src);
}
static INLINE int16_t read_16s(uint8_t *src) {
	return (int16_t)read_16u(src);
}
static INLINE int32_t read_32s(uint8_t *src) {
	return (int32_t)read_32u(src);
}
static INLINE int64_t read_64s(uint8_t *src) {
	return (int64_t)read_64u(src);
}

// f2dot14 type
typedef int16_t f2dot14;
static INLINE float caryll_from_f2dot14(int16_t x) {
	return x / 16384.0;
}
static INLINE int16_t caryll_to_f2dot14(float x) {
	return x * 16384.0;
}

// F16.16 (fixed) type
typedef int32_t f16dot16;
static INLINE float caryll_from_fixed(f16dot16 x) {
	return x / 65536.0;
}
static INLINE f16dot16 caryll_to_fixed(float x) {
	return x * 65536.0;
}

#include "handle.h"

// Handle types
typedef struct _caryll_handle glyph_handle;
typedef struct _caryll_handle fd_handle;
typedef struct _caryll_handle lookup_handle;

#define MOVE /*move*/

static INLINE json_value *preserialize(MOVE json_value *x) {
#ifdef CARYLL_USE_PRE_SERIALIZED
	json_serialize_opts opts = {.mode = json_serialize_mode_packed};
	size_t preserialize_len = json_measure_ex(x, opts);
	char *buf = (char *)malloc(preserialize_len);
	json_serialize_ex(buf, x, opts);
	json_builder_free(x);

	json_value *xx = json_string_new_length((uint32_t)(preserialize_len - 1), buf);
	xx->type = json_pre_serialized;
	return xx;
#else
	return x;
#endif
}

// Tag handler
static INLINE char *tag2str(uint32_t tag) {
	char *tags = (char *)malloc(sizeof(char) * 5);
	tags[0] = (tag >> 24) & 0xFF;
	tags[1] = (tag >> 16) & 0xFF;
	tags[2] = (tag >> 8) & 0xFF;
	tags[3] = tag & 0xFF;
	tags[4] = 0;
	return tags;
}

static INLINE uint32_t str2tag(char *tags) {
	if (!tags) return 0;
	uint32_t tag = 0;
	uint8_t len = 0;
	while (*tags && len < 4) {
		tag = (tag << 8) | (*tags), tags++, len++;
	}
	while (len < 4) {
		tag = (tag << 8) | ' ', len++;
	}
	return tag;
}

// Allocators

static INLINE void *__caryll_allocate(size_t n, unsigned long line) {
	if (!n) return NULL;
	void *p = malloc(n);
	if (!p) {
		fprintf(stderr, "[%ld]Out of memory(%ld bytes)\n", line, (unsigned long)n);
		exit(EXIT_FAILURE);
	}
	return p;
}
static INLINE void *__caryll_allocate_clean(size_t n, unsigned long line) {
	if (!n) return NULL;
	void *p = calloc(n, 1);
	if (!p) {
		fprintf(stderr, "[%ld]Out of memory(%ld bytes)\n", line, (unsigned long)n);
		exit(EXIT_FAILURE);
	}
	return p;
}
#ifdef __cplusplus
#define NEW(ptr) ptr = (decltype(ptr))__caryll_allocate(sizeof(decltype(*ptr)), __LINE__)
#define NEW_CLEAN(ptr) ptr = (decltype(ptr))__caryll_allocate_clean(sizeof(decltype(*ptr)), __LINE__)
#define NEW_N(ptr, n) ptr = (decltype(ptr))__caryll_allocate(sizeof(decltype(*ptr)) * (n), __LINE__)
#define FREE(ptr) (free(ptr), ptr = nullptr)
#define DELETE(fn, ptr) (fn(ptr), ptr = nullptr)
#else
#define NEW(ptr) ptr = __caryll_allocate(sizeof(*ptr), __LINE__)
#define NEW_CLEAN(ptr) ptr = __caryll_allocate_clean(sizeof(*ptr), __LINE__)
#define NEW_N(ptr, n) ptr = __caryll_allocate(sizeof(*ptr) * (n), __LINE__)
#define FREE(ptr) (free(ptr), ptr = NULL)
#define DELETE(fn, ptr) (fn(ptr), ptr = NULL)
#endif
#endif
