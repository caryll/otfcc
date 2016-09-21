#include "glyph-order.h"

caryll_GlyphOrder *caryll_new_GlyphOrder() {
	caryll_GlyphOrder *go;
	NEW(go);
	go->byGID = NULL;
	go->byName = NULL;
	return go;
}

void caryll_delete_GlyphOrder(caryll_GlyphOrder *go) {
	if (!go) return;
	caryll_GlyphOrderEntry *current, *temp;
	HASH_ITER(hhID, go->byGID, current, temp) {
		if (current->name) sdsfree(current->name);
		HASH_DELETE(hhID, go->byGID, current);
		HASH_DELETE(hhName, go->byName, current);
		FREE(current);
	}
	FREE(go);
}

// Register a gid->name map
sds caryll_setGlyphOrderByGID(caryll_GlyphOrder *go, glyphid_t gid, sds name) {
	caryll_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhID, go->byGID, &gid, sizeof(glyphid_t), s);
	if (s) {
		sdsfree(name);
		return s->name;
	} else {
		caryll_GlyphOrderEntry *t = NULL;
		HASH_FIND(hhName, go->byName, name, sdslen(name), t);
		if (t) {
			// The name is already in-use.
			sdsfree(name);
			name = sdscatprintf(sdsempty(), "gid%d", gid);
		}
		NEW(s);
		s->gid = gid;
		s->name = name;
		HASH_ADD(hhID, go->byGID, gid, sizeof(glyphid_t), s);
		HASH_ADD(hhName, go->byName, name[0], sdslen(s->name), s);
	}
	return name;
}

// Register a name->gid map
glyphid_t caryll_setGlyphOrderByName(caryll_GlyphOrder *go, sds name, glyphid_t gid) {
	caryll_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), s);
	if (s) {
		sdsfree(name);
		caryll_GlyphOrderEntry *t = NULL;
		HASH_FIND(hhID, go->byGID, &gid, sizeof(glyphid_t), t);
		if (!t) { HASH_ADD(hhID, go->byGID, gid, sizeof(glyphid_t), s); }
		return s->gid;
	} else {
		NEW(s);
		s->gid = gid;
		s->name = name;
		HASH_ADD(hhID, go->byGID, gid, sizeof(glyphid_t), s);
		HASH_ADD(hhName, go->byName, name[0], sdslen(s->name), s);
		return s->gid;
	}
}

// Register a name->(orderType, orderEntry) map.
void caryll_setGlyphOrderByNameWithOrder(caryll_GlyphOrder *go, sds name, uint8_t orderType, uint32_t orderEntry) {
	caryll_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), s);
	if (!s) {
		NEW(s);
		s->gid = -1;
		s->name = name;
		s->orderType = orderType;
		s->orderEntry = orderEntry;
		HASH_ADD(hhName, go->byName, name[0], sdslen(s->name), s);
	} else if (s->orderType > orderType) {
		s->orderType = orderType;
		s->orderEntry = orderEntry;
	}
}

static int compare_glyphorder_entry_b(caryll_GlyphOrderEntry *a, caryll_GlyphOrderEntry *b) {
	if (a->orderType < b->orderType) return (-1);
	if (a->orderType > b->orderType) return (1);
	if (a->orderEntry < b->orderEntry) return (-1);
	if (a->orderEntry > b->orderEntry) return (1);
	return 0;
}

// Complete ClyphOrder
void caryll_orderGlyphs(caryll_GlyphOrder *go) {
	HASH_SRT(hhName, go->byName, compare_glyphorder_entry_b);
	caryll_GlyphOrderEntry *current, *temp;
	glyphid_t gid = 0;
	HASH_ITER(hhName, go->byName, current, temp) {
		current->gid = gid;
		HASH_ADD(hhID, go->byGID, gid, sizeof(glyphid_t), current);
		gid += 1;
	}
}

enum { ORD_GLYPHORDER = 1, ORD_NOTDEF = 2, ORD_CMAP = 3, ORD_GLYF = 4 };

static void placeOrderEntriesFromGlyf(json_value *table, caryll_GlyphOrder *go) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds gname = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		caryll_setGlyphOrderByNameWithOrder(go, gname, (strcmp(gname, ".notdef") == 0 ? ORD_NOTDEF : ORD_GLYF), j);
	}
}
static void placeOrderEntriesFromCmap(json_value *table, caryll_GlyphOrder *go) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds unicodeStr = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		json_value *item = table->u.object.values[j].value;
		int32_t unicode = atoi(unicodeStr);
		if (item->type == json_string && unicode > 0 && unicode <= 0x10FFFF) { // a valid unicode codepoint
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			caryll_setGlyphOrderByNameWithOrder(go, gname, ORD_CMAP, unicode);
		}
	}
}
static void placeOrderEntriesFromSubtable(json_value *table, caryll_GlyphOrder *go, bool zeroOnly) {
	uint32_t uplimit = table->u.array.length;
	if (uplimit >= 1 && zeroOnly) { uplimit = 1; }
	for (uint32_t j = 0; j < uplimit; j++) {
		json_value *item = table->u.array.values[j];
		if (item->type == json_string) {
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			caryll_setGlyphOrderByNameWithOrder(go, gname, ORD_GLYPHORDER, j);
		}
	}
}

caryll_GlyphOrder *caryll_parse_GlyphOrder(json_value *root, caryll_Options *options) {
	caryll_GlyphOrder *go = caryll_new_GlyphOrder();
	if (root->type != json_object) return go;
	json_value *table;

	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		placeOrderEntriesFromGlyf(table, go);
		if ((table = json_obj_get_type(root, "cmap", json_object))) { placeOrderEntriesFromCmap(table, go); }
		if ((table = json_obj_get_type(root, "glyph_order", json_array))) {
			placeOrderEntriesFromSubtable(table, go, options->ignore_glyph_order);
		}
	}
	caryll_orderGlyphs(go);
	return go;
}

void caryll_nameAFieldUsingGlyphOrder(caryll_GlyphOrder *go, glyphid_t gid, sds *field) {
	caryll_GlyphOrderEntry *t;
	HASH_FIND(hhID, go->byGID, &gid, sizeof(glyphid_t), t);
	if (t != NULL) {
		*field = t->name;
	} else {
		*field = NULL;
	}
}

void caryll_nameAHandleUsingGlyphOrder(caryll_GlyphOrder *go, glyph_handle *h) {
	if (h->state == HANDLE_STATE_NAME) sdsfree(h->name);
	caryll_nameAFieldUsingGlyphOrder(go, h->index, &(h->name));
	if (h->name) { h->state = HANDLE_STATE_CONSOLIDATED; }
}

void caryll_consolidateAHandleUsingGlyphOrder(caryll_GlyphOrder *go, glyph_handle *h) {
	caryll_GlyphOrderEntry *t;
	HASH_FIND(hhName, go->byName, h->name, sdslen(h->name), t);
	if (t) {
		handle_consolidateTo(h, t->gid, t->name);
	} else {
		handle_delete(h);
	}
}

caryll_GlyphOrderEntry *caryll_lookupName(caryll_GlyphOrder *go, sds name) {
	caryll_GlyphOrderEntry *t = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), t);
	return t;
}
