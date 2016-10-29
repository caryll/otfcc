#include "support/util.h"
#include "otfcc/font.h"
#include "table/all.h"
#include "otfcc/sfnt-builder.h"

otfcc_Font *otfcc_newFont() {
	otfcc_Font *font;
	NEW(font);
	return font;
}

void otfcc_deleteFont(otfcc_Font *font) {
	if (font->head) DELETE(free, font->head);
	if (font->hhea) DELETE(free, font->hhea);
	if (font->maxp) DELETE(otfcc_deleteMaxp, font->maxp);
	if (font->OS_2) DELETE(free, font->OS_2);
	if (font->name) DELETE(otfcc_deleteName, font->name);
	if (font->hmtx) DELETE(otfcc_deleteHmtx, font->hmtx);
	if (font->vmtx) DELETE(otfcc_deleteVmtx, font->vmtx);
	if (font->post) DELETE(otfcc_deletePost, font->post);
	if (font->hdmx) DELETE(otfcc_deleteHdmx, font->hdmx);
	if (font->vhea) DELETE(free, font->vhea);
	if (font->fpgm) DELETE(otfcc_deleteFpgm_prep, font->fpgm);
	if (font->prep) DELETE(otfcc_deleteFpgm_prep, font->prep);
	if (font->cvt_) DELETE(otfcc_deleteCvt, font->cvt_);
	if (font->gasp) DELETE(otfcc_deleteGasp, font->gasp);
	if (font->CFF_) DELETE(otfcc_deleteCFF, font->CFF_);
	if (font->glyf) DELETE(otfcc_deleteGlyf, font->glyf);
	if (font->cmap) DELETE(otfcc_deleteCmap, font->cmap);
	if (font->LTSH) DELETE(otfcc_deleteLTSH, font->LTSH);
	if (font->GSUB) DELETE(otfcc_deleteOtl, font->GSUB);
	if (font->GPOS) DELETE(otfcc_deleteOtl, font->GPOS);
	if (font->GDEF) DELETE(otfcc_deleteGDEF, font->GDEF);
	if (font->BASE) DELETE(otfcc_deleteBASE, font->BASE);
	if (font->VORG) DELETE(otfcc_deleteVORG, font->VORG);
	if (font->glyph_order) { DELETE(GlyphOrder.free, font->glyph_order); }
	if (font) FREE(font);
}
