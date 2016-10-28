#include "support/util.h"
#include "otfcc/font.h"
#include "unconsolidate.h"

static caryll_font_subtype decideFontSubtypeOTF(caryll_SplineFontContainer *sfnt, uint32_t index) {
	caryll_Packet packet = sfnt->packets[index];
	FOR_TABLE('CFF ', table) {
		return FONTTYPE_CFF;
	}
	return FONTTYPE_TTF;
}

static caryll_Font *readOtf(void *_sfnt, uint32_t index, const otfcc_Options *options) {
	caryll_SplineFontContainer *sfnt = (caryll_SplineFontContainer *)_sfnt;
	if (sfnt->count - 1 < index) {
		return NULL;
	} else {
		caryll_Font *font = caryll_new_Font();
		caryll_Packet packet = sfnt->packets[index];
		font->subtype = decideFontSubtypeOTF(sfnt, index);
		font->head = table_read_head(packet, options);
		font->maxp = table_read_maxp(packet, options);
		font->name = table_read_name(packet, options);
		font->OS_2 = table_read_OS_2(packet, options);
		font->post = table_read_post(packet, options);
		font->hhea = table_read_hhea(packet, options);
		font->cmap = table_read_cmap(packet, options);
		if (font->subtype == FONTTYPE_TTF) {
			font->hmtx = table_read_hmtx(packet, options, font->hhea, font->maxp);
			font->vhea = table_read_vhea(packet, options);
			if (font->vhea) font->vmtx = table_read_vmtx(packet, options, font->vhea, font->maxp);
			font->fpgm = table_read_fpgm_prep(packet, options, 'fpgm');
			font->prep = table_read_fpgm_prep(packet, options, 'prep');
			font->cvt_ = table_read_cvt(packet, options, 'cvt ');
			font->gasp = table_read_gasp(packet, options);
			font->LTSH = table_read_LTSH(packet, options);
			font->glyf = table_read_glyf(packet, options, font->head, font->maxp);
		} else {
			table_CFFAndGlyf cffpr = table_read_cff_and_glyf(packet, options, font->head);
			font->CFF_ = cffpr.meta;
			font->glyf = cffpr.glyphs;
			font->vhea = table_read_vhea(packet, options);
			if (font->vhea) {
				font->vmtx = table_read_vmtx(packet, options, font->vhea, font->maxp);
				font->VORG = table_read_VORG(packet, options);
			}
		}
		if (font->glyf) {
			font->GSUB = table_read_otl(packet, options, 'GSUB');
			font->GPOS = table_read_otl(packet, options, 'GPOS');
			font->GDEF = table_read_GDEF(packet, options);
		}
		font->BASE = table_read_BASE(packet, options);
		otfcc_unconsolidateFont(font, options);
		return font;
	}
}
static void disposeReader(otfcc_IFontBuilder *self) {
	free(self);
}
otfcc_IFontBuilder *otfcc_newOTFReader() {
	otfcc_IFontBuilder *reader;
	NEW(reader);
	reader->create = readOtf;
	reader->dispose = disposeReader;
	return reader;
}
