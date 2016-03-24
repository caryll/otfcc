#ifndef CARYLL_SUPPORT_UTIL_H
#define CARYLL_SUPPORT_UTIL_H

#include "../extern/uthash.h"
#include "../extern/json.h"
#include "../extern/sds.h"

#define FOR_TABLE(name, table)                                                                                         \
	for (int keep = 1, count = 0, __notfound = 1; __notfound && keep && count < packet.numTables;                      \
	     keep = !keep, count++)                                                                                        \
		for (caryll_piece table = (packet.pieces)[count]; keep; keep = !keep)                                          \
			if (table.tag == (name))                                                                                   \
				for (int k2 = 1; k2; k2 = 0, __notfound = 0)

#define foreach_hash(id, range) for (id = (range); id != NULL; id = id->hh.next)

json_value *json_obj_get(json_value *obj, const char *key);
json_value *json_obj_get_type(json_value *obj, const char *key, json_type type);

#endif
