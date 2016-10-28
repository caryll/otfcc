#include "support/util.h"
#include "otfcc/font.h"

static otfcc_font_subtype otfcc_decideFontSubtypeFromJson(json_value *root) {
	if (json_obj_get_type(root, "CFF_", json_object) != NULL) {
		return FONTTYPE_CFF;
	} else {
		return FONTTYPE_TTF;
	}
}

// The default glyph_order object is completed using a two-step construction
enum { ORD_GLYPHORDER = 1, ORD_NOTDEF = 2, ORD_CMAP = 3, ORD_GLYF = 4 };

// Register a name->(orderType, orderEntry) map.
static void setOrderByName(otfcc_GlyphOrder *go, sds name, uint8_t orderType, uint32_t orderEntry) {
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

static int compare_glyphorder_entry_b(otfcc_GlyphOrderEntry *a, otfcc_GlyphOrderEntry *b) {
	if (a->orderType < b->orderType) return (-1);
	if (a->orderType > b->orderType) return (1);
	if (a->orderEntry < b->orderEntry) return (-1);
	if (a->orderEntry > b->orderEntry) return (1);
	return 0;
}

// Complete ClyphOrder
static void orderGlyphs(otfcc_GlyphOrder *go) {
	HASH_SRT(hhName, go->byName, compare_glyphorder_entry_b);
	otfcc_GlyphOrderEntry *current, *temp;
	glyphid_t gid = 0;
	HASH_ITER(hhName, go->byName, current, temp) {
		current->gid = gid;
		HASH_ADD(hhID, go->byGID, gid, sizeof(glyphid_t), current);
		gid += 1;
	}
}

static void escalateGlyphOrderByName(otfcc_GlyphOrder *go, sds name, uint8_t orderType, uint32_t orderEntry) {
	otfcc_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), s);
	if (s && s->orderType > orderType) {
		s->orderType = orderType;
		s->orderEntry = orderEntry;
	}
}

static void placeOrderEntriesFromGlyf(json_value *table, otfcc_GlyphOrder *go) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds gname = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		setOrderByName(go, gname, (strcmp(gname, ".notdef") == 0 ? ORD_NOTDEF : ORD_GLYF), j);
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
			escalateGlyphOrderByName(go, gname, ORD_CMAP, unicode);
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
			escalateGlyphOrderByName(go, gname, ORD_GLYPHORDER, j);
			sdsfree(gname);
		}
	}
}

static otfcc_GlyphOrder *parseGlyphOrder(json_value *root, const otfcc_Options *options) {
	otfcc_GlyphOrder *go = otfcc_newGlyphOrder();
	if (root->type != json_object) return go;
	json_value *table;

	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		placeOrderEntriesFromGlyf(table, go);
		if ((table = json_obj_get_type(root, "cmap", json_object))) { placeOrderEntriesFromCmap(table, go); }
		if ((table = json_obj_get_type(root, "glyph_order", json_array))) {
			placeOrderEntriesFromSubtable(table, go, options->ignore_glyph_order);
		}
	}
	orderGlyphs(go);
	return go;
}

static otfcc_Font *readJson(void *_root, uint32_t index, const otfcc_Options *options) {
	json_value *root = (json_value *)_root;
	otfcc_Font *font = otfcc_newFont();
	if (!font) return NULL;
	font->subtype = otfcc_decideFontSubtypeFromJson(root);
	font->glyph_order = parseGlyphOrder(root, options);
	font->glyf = otfcc_parseGlyf(root, font->glyph_order, options);
	font->CFF_ = otfcc_parseCFF(root, options);
	font->head = otfcc_parseHead(root, options);
	font->hhea = otfcc_parseHhea(root, options);
	font->OS_2 = otfcc_parseOS_2(root, options);
	font->maxp = otfcc_parseMaxp(root, options);
	font->post = otfcc_parsePost(root, options);
	font->name = otfcc_parseName(root, options);
	font->cmap = otfcc_parseCmap(root, options);
	if (!options->ignore_hints) {
		font->fpgm = otfcc_parseFpgmPrep(root, options, "fpgm");
		font->prep = otfcc_parseFpgmPrep(root, options, "prep");
		font->cvt_ = otfcc_parseCvt(root, options, "cvt_");
		font->gasp = otfcc_parseGasp(root, options);
	}
	font->vhea = otfcc_parseVhea(root, options);
	if (font->glyf) {
		font->GSUB = otfcc_parseOtl(root, options, "GSUB");
		font->GPOS = otfcc_parseOtl(root, options, "GPOS");
		font->GDEF = otfcc_parseGDEF(root, options);
	}
	font->BASE = otfcc_parseBASE(root, options);
	return font;
}
static void disposeReader(otfcc_IFontBuilder *self) {
	free(self);
}
otfcc_IFontBuilder *otfcc_newJsonReader() {
	otfcc_IFontBuilder *reader;
	NEW(reader);
	reader->create = readJson;
	reader->dispose = disposeReader;
	return reader;
}
