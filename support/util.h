#ifndef CARYLL_SUPPORT_UTIL_H
#define CARYLL_SUPPORT_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../extern/uthash.h"
#include "../extern/json.h"
#include "../extern/json-builder.h"
#include "../extern/sds.h"

#ifdef _MSC_VER
#define INLINE __forceinline /* use __forceinline (VC++ specific) */
#else
#define INLINE inline /* use standard inline */
#endif

#define FOR_TABLE(name, table)                                                                                         \
	for (int keep = 1, count = 0, __notfound = 1; __notfound && keep && count < packet.numTables;                      \
	     keep = !keep, count++)                                                                                        \
		for (caryll_piece table = (packet.pieces)[count]; keep; keep = !keep)                                          \
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
			if (cv && cv->type == json_integer) return cv->u.integer;
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
			if (cv && cv->type == json_integer) return cv->u.integer;
			if (cv && cv->type == json_double) return cv->u.dbl;
		}
	}
	return fallback;
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

static INLINE uint8_t caryll_blt8u(uint8_t *src) {
	return src[0];
}
static INLINE uint16_t caryll_blt16u(uint8_t *src) {
	uint16_t b0 = ((uint16_t)src[0]) << 8;
	uint16_t b1 = ((uint16_t)src[1]);
	return (b0 | b1);
}
static INLINE uint32_t caryll_blt32u(uint8_t *src) {
	uint32_t b0 = ((uint32_t)src[0]) << 24;
	uint32_t b1 = ((uint32_t)src[1]) << 16;
	uint32_t b2 = ((uint32_t)src[2]) << 8;
	uint32_t b3 = ((uint32_t)src[3]);
	return (b0 | b1 | b2 | b3);
}
static INLINE uint64_t caryll_blt64u(uint8_t *src) {
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
static INLINE int8_t caryll_blt8s(uint8_t *src) {
	return (int8_t)caryll_blt8u(src);
}
static INLINE int16_t caryll_blt16s(uint8_t *src) {
	return (int16_t)caryll_blt16u(src);
}
static INLINE int32_t caryll_blt32s(uint8_t *src) {
	return (int32_t)caryll_blt32u(src);
}
static INLINE int64_t caryll_blt64s(uint8_t *src) {
	return (int64_t)caryll_blt64u(src);
}
static INLINE float caryll_from_f2dot14(int16_t x) {
	return x / 16384.0;
}

// glyph reference type
typedef struct {
	uint16_t gid;
	sds name;
} glyph_handle;

#endif
