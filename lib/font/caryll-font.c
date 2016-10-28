#include "support/util.h"
#include "otfcc/font.h"
#include "otfcc/sfnt-builder.h"

caryll_Font *caryll_new_Font() {
	caryll_Font *font = calloc(1, sizeof(caryll_Font));
	if (!font) return NULL;
	font->head = NULL;
	font->hhea = NULL;
	font->maxp = NULL;
	font->hmtx = NULL;
	font->post = NULL;
	font->name = NULL;
	font->hdmx = NULL;
	font->glyf = NULL;
	font->cmap = NULL;
	font->glyph_order = NULL;
	font->fpgm = NULL;
	font->prep = NULL;
	font->cvt_ = NULL;
	font->gasp = NULL;
	font->vhea = NULL;
	font->vmtx = NULL;
	font->GSUB = NULL;
	font->GPOS = NULL;
	font->GDEF = NULL;
	font->BASE = NULL;
	font->VORG = NULL;
	return font;
}

void caryll_delete_Font(caryll_Font *font) {
	if (font->head) DELETE(free, font->head);
	if (font->hhea) DELETE(free, font->hhea);
	if (font->maxp) DELETE(table_delete_maxp, font->maxp);
	if (font->OS_2) DELETE(free, font->OS_2);
	if (font->name) DELETE(table_delete_name, font->name);
	if (font->hmtx) DELETE(table_delete_hmtx, font->hmtx);
	if (font->vmtx) DELETE(table_delete_vmtx, font->vmtx);
	if (font->post) DELETE(table_delete_post, font->post);
	if (font->hdmx) DELETE(table_delete_hdmx, font->hdmx);
	if (font->vhea) DELETE(free, font->vhea);
	if (font->fpgm) DELETE(table_delete_fpgm_prep, font->fpgm);
	if (font->prep) DELETE(table_delete_fpgm_prep, font->prep);
	if (font->cvt_) DELETE(table_delete_cvt, font->cvt_);
	if (font->gasp) DELETE(table_delete_gasp, font->gasp);
	if (font->CFF_) DELETE(table_delete_CFF, font->CFF_);
	if (font->glyf) DELETE(table_delete_glyf, font->glyf);
	if (font->cmap) DELETE(table_delete_cmap, font->cmap);
	if (font->LTSH) DELETE(table_delete_LTSH, font->LTSH);
	if (font->GSUB) DELETE(table_delete_otl, font->GSUB);
	if (font->GPOS) DELETE(table_delete_otl, font->GPOS);
	if (font->GDEF) DELETE(table_delete_GDEF, font->GDEF);
	if (font->BASE) DELETE(table_delete_BASE, font->BASE);
	if (font->VORG) DELETE(table_delete_VORG, font->VORG);
	if (font->glyph_order) { DELETE(caryll_delete_GlyphOrder, font->glyph_order); }
	if (font) free(font);
}
