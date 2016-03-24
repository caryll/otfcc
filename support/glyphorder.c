#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

#include "aglfn.h"

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

void caryll_name_glyphs(caryll_font *font) {
	glyph_order_hash *glyph_order = malloc(sizeof(glyph_order_hash));
	*glyph_order = NULL;

	glyph_order_hash *aglfn = malloc(sizeof(glyph_order_hash));
	*aglfn = NULL;
	setup_aglfn_glyph_names(aglfn);

	uint16_t numGlyphs = font->glyf->numberGlyphs;

	// pass 1: Map to `post` names
	if (font->post != NULL && font->post->post_name_map != NULL) {
		glyph_order_entry *s;
		foreach_hash(s, *font->post->post_name_map) { try_name_glyph(glyph_order, s->gid, sdsdup(s->name)); }
	}
	// pass 2: Map to AGLFN & Unicode
	if (font->cmap != NULL) {
		cmap_entry *s;
		foreach_hash(s, *font->cmap) if (s->glyph.gid > 0) {
			sds name = NULL;
			lookup_name(aglfn, s->unicode, &name);
			if (name == NULL) {
				name = sdscatprintf(sdsempty(), "uni%04X", s->unicode);
			} else {
				name = sdsdup(name);
			}
			int actuallyNamed = try_name_glyph(glyph_order, s->glyph.gid, name);
			if (!actuallyNamed) sdsfree(name);
		}
	}
	// pass 3: Map to GID
	for (uint16_t j = 0; j < numGlyphs; j++) {
		sds name = sdscatfmt(sdsempty(), "glyph%u", j);
		int actuallyNamed = try_name_glyph(glyph_order, j, name);
		if (!actuallyNamed) sdsfree(name);
	}

	delete_glyph_order_map(aglfn);

	font->glyph_order = glyph_order;
}

void caryll_name_cmap_entries(caryll_font *font) {
	if (font->glyph_order != NULL && font->cmap != NULL) {
		cmap_entry *s;
		foreach_hash(s, *font->cmap) { lookup_name(font->glyph_order, s->glyph.gid, &s->glyph.name); }
	}
}
void caryll_name_glyf(caryll_font *font) {
	if (font->glyph_order != NULL && font->glyf != NULL) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_glyph *g = font->glyf->glyphs[j];
			lookup_name(font->glyph_order, j, &g->name);
			if (g->numberOfReferences > 0 && g->references != NULL) {
				for (uint16_t k = 0; k < g->numberOfReferences; k++) {
					lookup_name(font->glyph_order, g->references[k].glyph.gid, &g->references[k].glyph.name);
				}
			}
		}
	}
}

void caryll_glyphorder_to_json(caryll_font *font, json_value *root) {
	if (!font->glyf) return;
	json_value *order = json_array_new(font->glyf->numberGlyphs);
	for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
		json_array_push(order,
		                json_string_new_length(sdslen(font->glyf->glyphs[j]->name), font->glyf->glyphs[j]->name));
	}
	json_object_push(root, "glyph_order", order);
}

static void caryll_glyphorder_from_json_order_subtable(glyph_order_hash *hash, json_value *table) {
	for (uint32_t j = 0; j < table->u.array.length; j++) {
		json_value *item = table->u.array.values[j];
		if (item->type == json_string) {
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			glyph_order_entry *item = NULL;
			HASH_FIND_STR(*hash, gname, item);
			if (item) {
				// found an entry
				if (item->dump_order_type > dump_order_type_glyphorder) {
					item->dump_order_type = dump_order_type_glyphorder;
					item->dump_order_entry = j;
				}
			} else {
				item = calloc(1, sizeof(glyph_order_entry));
				item->dump_order_type = dump_order_type_glyphorder;
				item->dump_order_entry = j;
				item->name = sdsdup(gname);
				HASH_ADD_STR(*hash, name, item);
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
				// found an entry
				if (item->dump_order_type > dump_order_type_cmap) {
					item->dump_order_type = dump_order_type_cmap;
					item->dump_order_entry = unicode;
				}
			} else {
				item = calloc(1, sizeof(glyph_order_entry));
				item->dump_order_type = dump_order_type_cmap;
				item->dump_order_entry = unicode;
				item->name = sdsdup(gname);
				HASH_ADD_STR(*hash, name, item);
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
		if (item) {
			// found an entry
			if (item->dump_order_type > dump_order_type_glyf) {
				item->dump_order_type = dump_order_type_glyf;
				item->dump_order_entry = j;
			}
		} else {
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

void caryll_glyphorder_from_json(caryll_font *font, json_value *root) {
	if (root->type != json_object) return;
	glyph_order_hash hash = NULL;
	json_value *table;
	if ((table = json_obj_get_type(root, "glyph_order", json_array))) {
		caryll_glyphorder_from_json_order_subtable(&hash, table);
	}
	if ((table = json_obj_get_type(root, "cmap", json_object))) {
		caryll_glyphorder_from_json_order_cmap(&hash, table);
	}
	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		caryll_glyphorder_from_json_order_glyf(&hash, table);
	}
	HASH_SORT(hash, compare_glyphorder_entry_b);
	glyph_order_entry *item;
	uint32_t j = 0;
	foreach_hash(item, hash) { item->gid = j++; }
	font->glyph_order = malloc(sizeof(glyph_order_hash));
	*font->glyph_order = hash;
}
