#include "support/util.h"
#include "otfcc/font.h"
#include "otfcc/sfnt-builder.h"

otfcc_Font *otfcc_new_Font() {
	otfcc_Font *font;
	NEW(font);
	return font;
}

void otfcc_delete_Font(otfcc_Font *font) {
	if (font->head) DELETE(free, font->head);
	if (font->hhea) DELETE(free, font->hhea);
	if (font->maxp) DELETE(otfcc_deleteTablemaxp, font->maxp);
	if (font->OS_2) DELETE(free, font->OS_2);
	if (font->name) DELETE(otfcc_deleteTablename, font->name);
	if (font->hmtx) DELETE(otfcc_deleteTablehmtx, font->hmtx);
	if (font->vmtx) DELETE(otfcc_deleteTablevmtx, font->vmtx);
	if (font->post) DELETE(otfcc_deleteTablepost, font->post);
	if (font->hdmx) DELETE(otfcc_deleteTablehdmx, font->hdmx);
	if (font->vhea) DELETE(free, font->vhea);
	if (font->fpgm) DELETE(otfcc_deleteTablefpgm_prep, font->fpgm);
	if (font->prep) DELETE(otfcc_deleteTablefpgm_prep, font->prep);
	if (font->cvt_) DELETE(otfcc_deleteTablecvt, font->cvt_);
	if (font->gasp) DELETE(otfcc_deleteTablegasp, font->gasp);
	if (font->CFF_) DELETE(otfcc_deleteTableCFF, font->CFF_);
	if (font->glyf) DELETE(otfcc_deleteTableglyf, font->glyf);
	if (font->cmap) DELETE(otfcc_deleteTablecmap, font->cmap);
	if (font->LTSH) DELETE(otfcc_deleteTableLTSH, font->LTSH);
	if (font->GSUB) DELETE(otfcc_deleteTableotl, font->GSUB);
	if (font->GPOS) DELETE(otfcc_deleteTableotl, font->GPOS);
	if (font->GDEF) DELETE(otfcc_deleteTableGDEF, font->GDEF);
	if (font->BASE) DELETE(otfcc_deleteTableBASE, font->BASE);
	if (font->VORG) DELETE(otfcc_deleteTableVORG, font->VORG);
	if (font->glyph_order) { DELETE(otfcc_delete_GlyphOrder, font->glyph_order); }
	if (font) FREE(font);
}
