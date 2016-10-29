#include "support/util.h"
#include "otfcc/font.h"
#include "table/all.h"
#include "otfcc/sfnt-builder.h"

otfcc_Font *otfcc_newFont() {
	otfcc_Font *font;
	NEW(font);
	return font;
}

void otfcc_deleteFontTable(otfcc_Font *font, const uint32_t tag) {
	switch (tag) {
		case 'head':
			if (font->head) DELETE(free, font->head);
			return;
		case 'hhea':
			if (font->hhea) DELETE(free, font->hhea);
			return;
		case 'maxp':
			if (font->maxp) DELETE(otfcc_deleteMaxp, font->maxp);
			return;
		case 'OS_2':
		case 'OS/2':
			if (font->OS_2) DELETE(free, font->OS_2);
			return;
		case 'name':
			if (font->name) DELETE(otfcc_deleteName, font->name);
			return;
		case 'hmtx':
			if (font->hmtx) DELETE(otfcc_deleteHmtx, font->hmtx);
			return;
		case 'vmtx':
			if (font->vmtx) DELETE(otfcc_deleteVmtx, font->vmtx);
			return;
		case 'post':
			if (font->post) DELETE(otfcc_deletePost, font->post);
			return;
		case 'hdmx':
			if (font->hdmx) DELETE(otfcc_deleteHdmx, font->hdmx);
			return;
		case 'vhea':
			if (font->vhea) DELETE(free, font->vhea);
			return;
		case 'fpgm':
			if (font->fpgm) DELETE(otfcc_deleteFpgm_prep, font->fpgm);
			return;
		case 'prep':
			if (font->prep) DELETE(otfcc_deleteFpgm_prep, font->prep);
			return;
		case 'cvt_':
		case 'cvt ':
			if (font->cvt_) DELETE(otfcc_deleteCvt, font->cvt_);
			return;
		case 'gasp':
			if (font->gasp) DELETE(otfcc_deleteGasp, font->gasp);
			return;
		case 'CFF_':
		case 'CFF ':
			if (font->CFF_) DELETE(otfcc_deleteCFF, font->CFF_);
			return;
		case 'glyf':
			if (font->glyf) DELETE(otfcc_deleteGlyf, font->glyf);
			return;
		case 'cmap':
			if (font->cmap) DELETE(otfcc_deleteCmap, font->cmap);
			return;
		case 'LTSH':
			if (font->LTSH) DELETE(otfcc_deleteLTSH, font->LTSH);
			return;
		case 'GSUB':
			if (font->GSUB) DELETE(otfcc_deleteOtl, font->GSUB);
			return;
		case 'GPOS':
			if (font->GPOS) DELETE(otfcc_deleteOtl, font->GPOS);
			return;
		case 'GDEF':
			if (font->GDEF) DELETE(otfcc_deleteGDEF, font->GDEF);
			return;
		case 'BASE':
			if (font->BASE) DELETE(otfcc_deleteBASE, font->BASE);
			return;
		case 'VORG':
			if (font->VORG) DELETE(otfcc_deleteVORG, font->VORG);
			return;
		case '$GRD':
			if (font->glyph_order) DELETE(GlyphOrder.free, font->glyph_order);
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
