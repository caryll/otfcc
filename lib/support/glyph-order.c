#include "util.h"
#include "otfcc/glyph-order.h"

otfcc_GlyphOrder *otfcc_new_GlyphOrder() {
	otfcc_GlyphOrder *go;
	NEW(go);
	go->byGID = NULL;
	go->byName = NULL;
	return go;
}

void otfcc_delete_GlyphOrder(otfcc_GlyphOrder *go) {
	if (!go) return;
	otfcc_GlyphOrderEntry *current, *temp;
	HASH_ITER(hhID, go->byGID, current, temp) {
		if (current->name) sdsfree(current->name);
		HASH_DELETE(hhID, go->byGID, current);
		HASH_DELETE(hhName, go->byName, current);
		FREE(current);
	}
	FREE(go);
}

// Register a gid->name map
sds otfcc_setGlyphOrderByGID(otfcc_GlyphOrder *go, glyphid_t gid, sds name) {
	otfcc_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhID, go->byGID, &gid, sizeof(glyphid_t), s);
	if (s) {
		// gid is already in the order table.
		// reject this naming suggestion.
		sdsfree(name);
		return s->name;
	} else {
		otfcc_GlyphOrderEntry *t = NULL;
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
glyphid_t otfcc_setGlyphOrderByName(otfcc_GlyphOrder *go, sds name, glyphid_t gid) {
	otfcc_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), s);
	if (s) {
		sdsfree(name);
		otfcc_GlyphOrderEntry *t = NULL;
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
void otfcc_setGlyphOrderByNameWithOrder(otfcc_GlyphOrder *go, sds name, uint8_t orderType, uint32_t orderEntry) {
	otfcc_GlyphOrderEntry *s = NULL;
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
static void otfcc_escalateGlyphOrderByNameWithOrder(otfcc_GlyphOrder *go, sds name, uint8_t orderType,
                                                     uint32_t orderEntry) {
	otfcc_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), s);
	if (s && s->orderType > orderType) {
		s->orderType = orderType;
		s->orderEntry = orderEntry;
	}
}

static int compare_glyphorder_entry_b(otfcc_GlyphOrderEntry *a, otfcc_GlyphOrderEntry *b) {
	if (a->orderType < b->orderType) return (-1);
	if (a->orderType > b->orderType) return (1);
	if (a->orderEntry < b->orderEntry) return (-1);
	if (a->orderEntry > b->orderEntry) return (1);
	return 0;
}

// Complete ClyphOrder
void otfcc_orderGlyphs(otfcc_GlyphOrder *go) {
	HASH_SRT(hhName, go->byName, compare_glyphorder_entry_b);
	otfcc_GlyphOrderEntry *current, *temp;
	glyphid_t gid = 0;
	HASH_ITER(hhName, go->byName, current, temp) {
		current->gid = gid;
		HASH_ADD(hhID, go->byGID, gid, sizeof(glyphid_t), current);
		gid += 1;
	}
}

enum { ORD_GLYPHORDER = 1, ORD_NOTDEF = 2, ORD_CMAP = 3, ORD_GLYF = 4 };

static void placeOrderEntriesFromGlyf(json_value *table, otfcc_GlyphOrder *go) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds gname = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		otfcc_setGlyphOrderByNameWithOrder(go, gname, (strcmp(gname, ".notdef") == 0 ? ORD_NOTDEF : ORD_GLYF), j);
	}
}
static void placeOrderEntriesFromCmap(json_value *table, otfcc_GlyphOrder *go) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds unicodeStr = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		json_value *item = table->u.object.values[j].value;
		int32_t unicode = atoi(unicodeStr);
		sdsfree(unicodeStr);
		if (item->type == json_string && unicode > 0 && unicode <= 0x10FFFF) { // a valid unicode codepoint
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			otfcc_escalateGlyphOrderByNameWithOrder(go, gname, ORD_CMAP, unicode);
			sdsfree(gname);
		}
	}
}
static void placeOrderEntriesFromSubtable(json_value *table, otfcc_GlyphOrder *go, bool zeroOnly) {
	uint32_t uplimit = table->u.array.length;
	if (uplimit >= 1 && zeroOnly) { uplimit = 1; }
	for (uint32_t j = 0; j < uplimit; j++) {
		json_value *item = table->u.array.values[j];
		if (item->type == json_string) {
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			otfcc_escalateGlyphOrderByNameWithOrder(go, gname, ORD_GLYPHORDER, j);
			sdsfree(gname);
		}
	}
}

otfcc_GlyphOrder *otfcc_parse_GlyphOrder(json_value *root, const otfcc_Options *options) {
	otfcc_GlyphOrder *go = otfcc_new_GlyphOrder();
	if (root->type != json_object) return go;
	json_value *table;

	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		placeOrderEntriesFromGlyf(table, go);
		if ((table = json_obj_get_type(root, "cmap", json_object))) { placeOrderEntriesFromCmap(table, go); }
		if ((table = json_obj_get_type(root, "glyph_order", json_array))) {
			placeOrderEntriesFromSubtable(table, go, options->ignore_glyph_order);
		}
	}
	otfcc_orderGlyphs(go);
	return go;
}

bool gord_nameAFieldShared(otfcc_GlyphOrder *go, glyphid_t gid, sds *field) {
	otfcc_GlyphOrderEntry *t;
	HASH_FIND(hhID, go->byGID, &gid, sizeof(glyphid_t), t);
	if (t != NULL) {
		*field = t->name;
		return true;
	} else {
		*field = NULL;
		return false;
	}
}

bool gord_consolidateHandle(otfcc_GlyphOrder *go, glyph_handle *h) {
	if (h->state == HANDLE_STATE_CONSOLIDATED) {
		otfcc_GlyphOrderEntry *t;
		HASH_FIND(hhName, go->byName, h->name, sdslen(h->name), t);
		if (t) {
			handle_consolidateTo(h, t->gid, t->name);
			return true;
		}
		HASH_FIND(hhName, go->byGID, &(h->index), sizeof(glyphid_t), t);
		if (t) {
			handle_consolidateTo(h, t->gid, t->name);
			return true;
		}
	} else if (h->state == HANDLE_STATE_NAME) {
		otfcc_GlyphOrderEntry *t;
		HASH_FIND(hhName, go->byName, h->name, sdslen(h->name), t);
		if (t) {
			handle_consolidateTo(h, t->gid, t->name);
			return true;
		}
	} else if (h->state == HANDLE_STATE_INDEX) {
		sds name = NULL;
		gord_nameAFieldShared(go, h->index, &name);
		if (name) {
			handle_consolidateTo(h, h->index, name);
			return true;
		}
	}
	return false;
}

otfcc_GlyphOrderEntry *otfcc_lookupName(otfcc_GlyphOrder *go, sds name) {
	otfcc_GlyphOrderEntry *t = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), t);
	return t;
}

otfcc_GlyphHandle gord_formIndexedHandle(otfcc_GlyphOrder *go, glyphid_t gid) {
	otfcc_GlyphOrderEntry *t;
	HASH_FIND(hhID, go->byGID, &gid, sizeof(glyphid_t), t);
	if (t) {
		return handle_fromConsolidated(t->gid, t->name);
	} else {
		return handle_new();
	}
}

otfcc_GlyphHandle gord_formNamedHandle(otfcc_GlyphOrder *go, const sds name) {
	otfcc_GlyphOrderEntry *t;
	HASH_FIND(hhName, go->byName, name, sdslen(name), t);
	if (t) {
		return handle_fromConsolidated(t->gid, t->name);
	} else {
		return handle_new();
	}
}
