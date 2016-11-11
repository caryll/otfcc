#include "support/util.h"
#include "otfcc/font.h"
#include "table/all.h"
#include "otfcc/sfnt-builder.h"

otfcc_Font *otfcc_newFont() {
	otfcc_Font *font;
	NEW(font);
	return font;
}

void *otfcc_createFontTable(otfcc_Font *font, const uint32_t tag) {
	switch (tag) {
		case 'name':
			return iTable_name.create();
		case 'GSUB':
		case 'GPOS':
			return iTable_OTL.create();
		default:
			return NULL;
	}
}

void otfcc_deleteFontTable(otfcc_Font *font, const uint32_t tag) {
	switch (tag) {
		case 'head':
			if (font->head) DELETE(iTable_head.destroy, font->head);
			return;
		case 'hhea':
			if (font->hhea) DELETE(iTable_hhea.destroy, font->hhea);
			return;
		case 'maxp':
			if (font->maxp) DELETE(iTable_maxp.destroy, font->maxp);
			return;
		case 'OS_2':
		case 'OS/2':
			if (font->OS_2) DELETE(iTable_OS_2.destroy, font->OS_2);
			return;
		case 'name':
			if (font->name) DELETE(iTable_name.destroy, font->name);
			return;
		case 'hmtx':
			if (font->hmtx) DELETE(iTable_hmtx.destroy, font->hmtx);
			return;
		case 'vmtx':
			if (font->vmtx) DELETE(iTable_vmtx.destroy, font->vmtx);
			return;
		case 'post':
			if (font->post) DELETE(iTable_post.destroy, font->post);
			return;
#if 0
		case 'hdmx':
			if (font->hdmx) DELETE(otfcc_deleteHdmx, font->hdmx);
			return;
#endif
		case 'vhea':
			if (font->vhea) DELETE(iTable_vhea.destroy, font->vhea);
			return;
		case 'fpgm':
			if (font->fpgm) DELETE(iTable_fpgm_prep.destroy, font->fpgm);
			return;
		case 'prep':
			if (font->prep) DELETE(iTable_fpgm_prep.destroy, font->prep);
			return;
		case 'cvt_':
		case 'cvt ':
			if (font->cvt_) DELETE(iTable_cvt.destroy, font->cvt_);
			return;
		case 'gasp':
			if (font->gasp) DELETE(iTable_gasp.destroy, font->gasp);
			return;
		case 'CFF_':
		case 'CFF ':
			if (font->CFF_) DELETE(iTable_CFF.destroy, font->CFF_);
			return;
		case 'glyf':
			if (font->glyf) DELETE(iTable_glyf.destroy, font->glyf);
			return;
		case 'cmap':
			if (font->cmap) DELETE(iTable_cmap.destroy, font->cmap);
			return;
		case 'LTSH':
			if (font->LTSH) DELETE(iTable_LTSH.destroy, font->LTSH);
			return;
		case 'GSUB':
			if (font->GSUB) DELETE(iTable_OTL.destroy, font->GSUB);
			return;
		case 'GPOS':
			if (font->GPOS) DELETE(iTable_OTL.destroy, font->GPOS);
			return;
		case 'GDEF':
			if (font->GDEF) DELETE(iTable_GDEF.destroy, font->GDEF);
			return;
		case 'BASE':
			if (font->BASE) DELETE(iTable_BASE.destroy, font->BASE);
			return;
		case 'VORG':
			if (font->VORG) DELETE(otfcc_deleteVORG, font->VORG);
			return;
		case '$GRD':
			if (font->glyph_order) DELETE(GlyphOrder.destroy, font->glyph_order);
			return;
	}
}
void otfcc_deleteFont(otfcc_Font *font) {
	otfcc_deleteFontTable(font, 'head');
	otfcc_deleteFontTable(font, 'hhea');
	otfcc_deleteFontTable(font, 'maxp');
	otfcc_deleteFontTable(font, 'OS_2');
	otfcc_deleteFontTable(font, 'name');
	otfcc_deleteFontTable(font, 'hmtx');
	otfcc_deleteFontTable(font, 'vmtx');
	otfcc_deleteFontTable(font, 'post');
	otfcc_deleteFontTable(font, 'hdmx');
	otfcc_deleteFontTable(font, 'vhea');
	otfcc_deleteFontTable(font, 'fpgm');
	otfcc_deleteFontTable(font, 'prep');
	otfcc_deleteFontTable(font, 'cvt_');
	otfcc_deleteFontTable(font, 'gasp');
	otfcc_deleteFontTable(font, 'CFF_');
	otfcc_deleteFontTable(font, 'glyf');
	otfcc_deleteFontTable(font, 'cmap');
	otfcc_deleteFontTable(font, 'LTSH');
	otfcc_deleteFontTable(font, 'GSUB');
	otfcc_deleteFontTable(font, 'GPOS');
	otfcc_deleteFontTable(font, 'GDEF');
	otfcc_deleteFontTable(font, 'BASE');
	otfcc_deleteFontTable(font, 'VORG');
	otfcc_deleteFontTable(font, '$GRD');
	if (font) FREE(font);
}
