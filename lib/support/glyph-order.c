#include "util.h"
#include "otfcc/glyph-order.h"

otfcc_GlyphOrder *otfcc_newGlyphOrder() {
	otfcc_GlyphOrder *go;
	NEW(go);
	go->byGID = NULL;
	go->byName = NULL;
	return go;
}

void otfcc_deleteGlyphOrder(otfcc_GlyphOrder *go) {
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

bool otfcc_gordNameAFieldShared(otfcc_GlyphOrder *go, glyphid_t gid, sds *field) {
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

bool otfcc_gordConsolidateHandle(otfcc_GlyphOrder *go, glyph_handle *h) {
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
		otfcc_gordNameAFieldShared(go, h->index, &name);
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

otfcc_GlyphHandle otfcc_gordFormIndexedHandle(otfcc_GlyphOrder *go, glyphid_t gid) {
	otfcc_GlyphOrderEntry *t;
	HASH_FIND(hhID, go->byGID, &gid, sizeof(glyphid_t), t);
	if (t) {
		return handle_fromConsolidated(t->gid, t->name);
	} else {
		return handle_new();
	}
}

otfcc_GlyphHandle otfcc_gordFormNamedHandle(otfcc_GlyphOrder *go, const sds name) {
	otfcc_GlyphOrderEntry *t;
	HASH_FIND(hhName, go->byName, name, sdslen(name), t);
	if (t) {
		return handle_fromConsolidated(t->gid, t->name);
	} else {
		return handle_new();
	}
}
