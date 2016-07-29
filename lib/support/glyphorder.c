#include "glyphorder.h"

int try_name_glyph(glyph_order_hash *glyph_order, int _id, sds name) {
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

void lookup_name(glyph_order_hash *glyph_order, int _gid, sds *field) {
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

static int dump_order_type_glyphorder = 1;
static int dump_order_dotnotdef = 2;
static int dump_order_type_cmap = 3;
static int dump_order_type_glyf = 4;
static void caryll_glyphorder_from_json_order_subtable(glyph_order_hash *hash, json_value *table,
                                                       caryll_options *options) {
	uint32_t uplimit = table->u.array.length;
	if (uplimit >= 1 && options->ignore_glyph_order) { uplimit = 1; }
	for (uint32_t j = 0; j < uplimit; j++) {
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
			} else {
				fprintf(stderr, "[Glyphorder] Ignored missing glyph /%s in "
				                "glyph_order (mapped to %d).\n",
				        gname, j);
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
			} else {
				fprintf(stderr, "[Glyphorder] Ignored missing glyph /%s in "
				                "cmap (mapped to U+%04X).\n",
				        gname, unicode);
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
			if (strcmp(gname, ".notdef") == 0) {
				item->dump_order_type = dump_order_dotnotdef;
			} else {
				item->dump_order_type = dump_order_type_glyf;
			}
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

glyph_order_hash *caryll_glyphorder_from_json(json_value *root, caryll_options *options) {
	if (root->type != json_object) return NULL;
	glyph_order_hash hash = NULL;
	json_value *table;
	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		caryll_glyphorder_from_json_order_glyf(&hash, table);
		if ((table = json_obj_get_type(root, "glyph_order", json_array))) {
			caryll_glyphorder_from_json_order_subtable(&hash, table, options);
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
