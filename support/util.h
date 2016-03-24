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
    #define INLINE inline        /* use standard inline */
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

typedef struct {
	uint16_t gid;
	sds name;
} glyph_handle;

#endif
