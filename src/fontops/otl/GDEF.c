#include "GDEF.h"

typedef struct {
	int gid;
	sds name;
	caret_value_record cr;
	UT_hash_handle hh;
} GDEF_ligcaret_hash;
static int by_gid(GDEF_ligcaret_hash *a, GDEF_ligcaret_hash *b) { return a->gid - b->gid; }
void consolidate_GDEF(caryll_font *font, table_GDEF *gdef, char *tableName) {
	if (!font || !font->glyph_order || !gdef) return;
	if (gdef->glyphClassDef) consolidate_classdef(font, gdef->glyphClassDef, tableName);
	if (gdef->markAttachClassDef) consolidate_classdef(font, gdef->markAttachClassDef, tableName);
	if (gdef->ligCarets) {
		consolidate_coverage(font, gdef->ligCarets->coverage, tableName);
		GDEF_ligcaret_hash *h = NULL;
		for (uint16_t j = 0; j < gdef->ligCarets->coverage->numGlyphs; j++) {
			GDEF_ligcaret_hash *s;
			int gid = gdef->ligCarets->coverage->glyphs[j].gid;
			sds gname = gdef->ligCarets->coverage->glyphs[j].name;
			if (gname) {
				HASH_FIND_INT(h, &gid, s);
				if (!s) {
					NEW(s);
					s->gid = gid;
					s->name = gname;
					s->cr = gdef->ligCarets->carets[j];
					HASH_ADD_INT(h, gid, s);
				} else {
					free(gdef->ligCarets->carets[j].values);
				}
			}
		}
		HASH_SORT(h, by_gid);
		gdef->ligCarets->coverage->numGlyphs = HASH_COUNT(h);
		GDEF_ligcaret_hash *s, *tmp;
		uint16_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			gdef->ligCarets->coverage->glyphs[j].gid = s->gid;
			gdef->ligCarets->coverage->glyphs[j].name = s->name;
			gdef->ligCarets->carets[j] = s->cr;
			j++;
			HASH_DEL(h, s);
			free(s);
		}
	}
}
