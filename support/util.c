#include "util.h"
json_value *json_obj_get(json_value *obj, const char *key) {
	if (!obj || obj->type != json_object) return NULL;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		if (strcmp(ck, key) == 0) return obj->u.object.values[_k].value;
	}
	return NULL;
}
json_value *json_obj_get_type(json_value *obj, const char *key, json_type type) {
	json_value *v = json_obj_get(obj, key);
	if (v && v->type == type) return v;
	return NULL;
}
