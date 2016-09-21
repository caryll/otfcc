#ifndef CARYLL_SUPPORT_GLYPH_ORDER_H
#define CARYLL_SUPPORT_GLYPH_ORDER_H

#include "util.h"

typedef struct {
	glyphid_t gid;
	sds name;
	uint8_t orderType;
	uint32_t orderEntry;
	UT_hash_handle hhID;
	UT_hash_handle hhName;
} caryll_GlyphOrderEntry;

typedef struct {
	caryll_GlyphOrderEntry *byGID;
	caryll_GlyphOrderEntry *byName;
} caryll_GlyphOrder;

caryll_GlyphOrder *caryll_new_GlyphOrder();
void caryll_delete_GlyphOrder(caryll_GlyphOrder *go);
sds caryll_setGlyphOrderByGID(caryll_GlyphOrder *go, glyphid_t gid, sds name);
glyphid_t caryll_setGlyphOrderByName(caryll_GlyphOrder *go, sds name, glyphid_t gid);
void caryll_setGlyphOrderByNameWithOrder(caryll_GlyphOrder *go, sds name, uint8_t orderType, uint32_t orderEntry);
void caryll_orderGlyphs(caryll_GlyphOrder *go);
caryll_GlyphOrder *caryll_parse_GlyphOrder(json_value *root, caryll_Options *options);
void caryll_nameAFieldUsingGlyphOrder(caryll_GlyphOrder *go, glyphid_t gid, sds *field);
void caryll_nameAHandleUsingGlyphOrder(caryll_GlyphOrder *go, glyph_handle *h);
void caryll_consolidateAHandleUsingGlyphOrder(caryll_GlyphOrder *go, glyph_handle *h);
caryll_GlyphOrderEntry *caryll_lookupName(caryll_GlyphOrder *go, sds name);

#endif
