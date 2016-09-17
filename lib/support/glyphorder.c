#include "glyphorder.h"

int glyphorder_tryAssignName(glyphorder_Map *glyph_order, int _id, sds name) {
	glyphorder_Entry *s;
	int id = _id;
	HASH_FIND_INT(*glyph_order, &id, s);
	if (s == NULL) {
		s = malloc(sizeof(glyphorder_Entry));
		s->gid = id;
		s->name = name;
		HASH_ADD_INT(*glyph_order, gid, s);
		return 1;
	} else {
		return 0;
	}
}

void glyphorder_nameAnIndex(glyphorder_Map *glyph_order, int _gid, sds *field) {
	glyphorder_Entry *so;
	int gid = _gid;
	HASH_FIND_INT(*glyph_order, &gid, so);
	if (so != NULL) { *field = so->name; }
}

void glyphorder_nameAIndexedHandle(glyphorder_Map *glyph_order, glyph_handle *h) {
	if (h->state == HANDLE_STATE_NAME) sdsfree(h->name);
	glyphorder_nameAnIndex(glyph_order, h->index, &(h->name));
	if (h->name) { h->state = HANDLE_STATE_CONSOLIDATED; }
}

void glyphorder_deleteMap(glyphorder_Map *map) {
	glyphorder_Entry *s, *tmp;
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
static void caryll_parse_glyphorder_order_subtable(glyphorder_Map *hash, json_value *table, caryll_Options *options) {
	uint32_t uplimit = table->u.array.length;
	if (uplimit >= 1 && options->ignore_glyph_order) { uplimit = 1; }
	for (uint32_t j = 0; j < uplimit; j++) {
		json_value *item = table->u.array.values[j];
		if (item->type == json_string) {
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			glyphorder_Entry *item = NULL;
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
static void caryll_parse_glyphorder_order_cmap(glyphorder_Map *hash, json_value *table) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds unicodeStr = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		json_value *item = table->u.object.values[j].value;
		int32_t unicode = atoi(unicodeStr);
		if (item->type == json_string && unicode > 0 && unicode <= 0x10FFFF) { // a valid unicode codepoint
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			glyphorder_Entry *item = NULL;
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
static void caryll_parse_glyphorder_order_glyf(glyphorder_Map *hash, json_value *table) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds gname = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		glyphorder_Entry *item = NULL;
		HASH_FIND_STR(*hash, gname, item);
		if (!item) {
			item = calloc(1, sizeof(glyphorder_Entry));
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

static int compare_glyphorder_entry_b(glyphorder_Entry *a, glyphorder_Entry *b) {
	if (a->dump_order_type < b->dump_order_type) return (-1);
	if (a->dump_order_type > b->dump_order_type) return (1);
	if (a->dump_order_entry < b->dump_order_entry) return (-1);
	if (a->dump_order_entry > b->dump_order_entry) return (1);
	return 0;
}

glyphorder_Map *parse_glyphorder(json_value *root, caryll_Options *options) {
	if (root->type != json_object) return NULL;
	glyphorder_Map hash = NULL;
	json_value *table;
	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		caryll_parse_glyphorder_order_glyf(&hash, table);
		if ((table = json_obj_get_type(root, "glyph_order", json_array))) {
			caryll_parse_glyphorder_order_subtable(&hash, table, options);
		}
		if ((table = json_obj_get_type(root, "cmap", json_object))) {
			caryll_parse_glyphorder_order_cmap(&hash, table);
		}
	}
	HASH_SORT(hash, compare_glyphorder_entry_b);
	glyphorder_Entry *item;
	uint32_t j = 0;
	foreach_hash(item, hash) {
		item->gid = j++;
	}
	glyphorder_Map *go = malloc(sizeof(glyphorder_Map));
	*go = hash;
	return go;
}
