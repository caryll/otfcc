#ifndef CARYLL_INCLUDE_GLYPH_ORDER_H
#define CARYLL_INCLUDE_GLYPH_ORDER_H

#include "dep/sds.h"
#include "dep/json.h"
#include "dep/uthash.h"
#include "caryll/ownership.h"
#include "otfcc/primitives.h"
#include "otfcc/handle.h"
#include "otfcc/options.h"

typedef struct {
	glyphid_t gid;
	sds name;
	uint8_t orderType;
	uint32_t orderEntry;
	UT_hash_handle hhID;
	UT_hash_handle hhName;
} otfcc_GlyphOrderEntry;

typedef struct {
	otfcc_GlyphOrderEntry *byGID;
	otfcc_GlyphOrderEntry *byName;
} otfcc_GlyphOrder;

otfcc_GlyphOrder *otfcc_newGlyphOrder();
void otfcc_deleteGlyphOrder(otfcc_GlyphOrder *go);
sds otfcc_setGlyphOrderByGID(otfcc_GlyphOrder *go, glyphid_t gid, sds name);
glyphid_t otfcc_setGlyphOrderByName(otfcc_GlyphOrder *go, sds name, glyphid_t gid);
bool otfcc_gordNameAFieldShared(otfcc_GlyphOrder *go, glyphid_t gid, sds *field); // return a shared name pointer
bool otfcc_gordConsolidateHandle(otfcc_GlyphOrder *go, otfcc_GlyphHandle *h);
otfcc_GlyphOrderEntry *otfcc_lookupName(otfcc_GlyphOrder *go, sds name);

otfcc_GlyphHandle otfcc_gordFormIndexedHandle(otfcc_GlyphOrder *go, glyphid_t gid);
otfcc_GlyphHandle otfcc_gordFormNamedHandle(otfcc_GlyphOrder *go, const sds name);

#endif
