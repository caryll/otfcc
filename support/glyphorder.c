#include "glyphorder.h"

int try_name_glyph(glyph_order_hash *glyph_order, uint16_t _id, sds name) {
	glyph_order_entry *s;
	int id = _id;
	HASH_FIND_INT(*glyph_order, &id, s);
	if (s == NULL) {
		s = malloc(sizeof(glyph_order_entry));
		s->gid = id;
		s->name = name;
		HASH_ADD_INT(*glyph_order, gid, s);
		return 1;
	} else {
		return 0;
	}
}

void lookup_name(glyph_order_hash *glyph_order, uint16_t _gid, sds *field) {
	glyph_order_entry *so;
	int gid = _gid;
	HASH_FIND_INT(*glyph_order, &gid, so);
	if (so != NULL) { *field = so->name; }
}

void delete_glyph_order_map(glyph_order_hash *map) {
	glyph_order_entry *s, *tmp;
	HASH_ITER(hh, *map, s, tmp) {
		// delete and free all cmap entries
		sdsfree(s->name);
		HASH_DEL(*map, s);
		free(s);
	}
	free(map);
}

static void caryll_glyphorder_from_json_order_subtable(glyph_order_hash *hash, json_value *table) {
	for (uint32_t j = 0; j < table->u.array.length; j++) {
		json_value *item = table->u.array.values[j];
		if (item->type == json_string) {
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			glyph_order_entry *item = NULL;
			HASH_FIND_STR(*hash, gname, item);
			if (item) {
				// Escalate
				if (item->dump_order_type > dump_order_type_glyphorder) {
					item->dump_order_type = dump_order_type_glyphorder;
					item->dump_order_entry = j;
				}
			}
			sdsfree(gname);
		}
	}
}
static void caryll_glyphorder_from_json_order_cmap(glyph_order_hash *hash, json_value *table) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds unicodeStr = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		json_value *item = table->u.object.values[j].value;
		int32_t unicode = atoi(unicodeStr);
		if (item->type == json_string && unicode > 0 && unicode <= 0x10FFFF) { // a valid unicode codepoint
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			glyph_order_entry *item = NULL;
			HASH_FIND_STR(*hash, gname, item);
			if (item) {
				// Escalate
				if (item->dump_order_type > dump_order_type_cmap) {
					item->dump_order_type = dump_order_type_cmap;
					item->dump_order_entry = unicode;
				}
			}
			sdsfree(gname);
		}
		sdsfree(unicodeStr);
	}
}
static void caryll_glyphorder_from_json_order_glyf(glyph_order_hash *hash, json_value *table) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds gname = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		glyph_order_entry *item = NULL;
		HASH_FIND_STR(*hash, gname, item);
		if (!item) {
			item = calloc(1, sizeof(glyph_order_entry));
			item->dump_order_type = dump_order_type_glyf;
			item->dump_order_entry = j;
			item->name = sdsdup(gname);
			HASH_ADD_STR(*hash, name, item);
		}
		sdsfree(gname);
	}
}

static int compare_glyphorder_entry_b(glyph_order_entry *a, glyph_order_entry *b) {
	if (a->dump_order_type < b->dump_order_type) return (-1);
	if (a->dump_order_type > b->dump_order_type) return (1);
	if (a->dump_order_entry < b->dump_order_entry) return (-1);
	if (a->dump_order_entry > b->dump_order_entry) return (1);
	return 0;
}

glyph_order_hash *caryll_glyphorder_from_json(json_value *root) {
	if (root->type != json_object) return NULL;
	glyph_order_hash hash = NULL;
	json_value *table;
	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		caryll_glyphorder_from_json_order_glyf(&hash, table);
		if ((table = json_obj_get_type(root, "glyph_order", json_array))) {
			caryll_glyphorder_from_json_order_subtable(&hash, table);
		}
		if ((table = json_obj_get_type(root, "cmap", json_object))) {
			caryll_glyphorder_from_json_order_cmap(&hash, table);
		}
	}
	HASH_SORT(hash, compare_glyphorder_entry_b);
	glyph_order_entry *item;
	uint32_t j = 0;
	foreach_hash(item, hash) { item->gid = j++; }
	glyph_order_hash *go = malloc(sizeof(glyph_order_hash));
	*go = hash;
	return go;
}
