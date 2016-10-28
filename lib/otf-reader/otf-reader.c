#include "support/util.h"
#include "otfcc/font.h"
#include "unconsolidate.h"

static otfcc_font_subtype decideFontSubtypeOTF(otfcc_SplineFontContainer *sfnt, uint32_t index) {
	otfcc_Packet packet = sfnt->packets[index];
	FOR_TABLE('CFF ', table) {
		return FONTTYPE_CFF;
	}
	return FONTTYPE_TTF;
}

static otfcc_Font *readOtf(void *_sfnt, uint32_t index, const otfcc_Options *options) {
	otfcc_SplineFontContainer *sfnt = (otfcc_SplineFontContainer *)_sfnt;
	if (sfnt->count - 1 < index) {
		return NULL;
	} else {
		otfcc_Font *font = otfcc_new_Font();
		otfcc_Packet packet = sfnt->packets[index];
		font->subtype = decideFontSubtypeOTF(sfnt, index);
		font->head = otfcc_readTablehead(packet, options);
		font->maxp = otfcc_readTablemaxp(packet, options);
		font->name = otfcc_readTablename(packet, options);
		font->OS_2 = otfcc_readTableOS_2(packet, options);
		font->post = otfcc_readTablepost(packet, options);
		font->hhea = otfcc_readTablehhea(packet, options);
		font->cmap = otfcc_readTablecmap(packet, options);
		if (font->subtype == FONTTYPE_TTF) {
			font->hmtx = otfcc_readTablehmtx(packet, options, font->hhea, font->maxp);
			font->vhea = otfcc_readTablevhea(packet, options);
			if (font->vhea) font->vmtx = otfcc_readTablevmtx(packet, options, font->vhea, font->maxp);
			font->fpgm = otfcc_readTablefpgm_prep(packet, options, 'fpgm');
			font->prep = otfcc_readTablefpgm_prep(packet, options, 'prep');
			font->cvt_ = otfcc_readTablecvt(packet, options, 'cvt ');
			font->gasp = otfcc_readTablegasp(packet, options);
			font->LTSH = otfcc_readTableLTSH(packet, options);
			font->glyf = otfcc_readTableglyf(packet, options, font->head, font->maxp);
		} else {
			table_CFFAndGlyf cffpr = otfcc_readTablecff_and_glyf(packet, options, font->head);
			font->CFF_ = cffpr.meta;
			font->glyf = cffpr.glyphs;
			font->vhea = otfcc_readTablevhea(packet, options);
			if (font->vhea) {
				font->vmtx = otfcc_readTablevmtx(packet, options, font->vhea, font->maxp);
				font->VORG = otfcc_readTableVORG(packet, options);
			}
		}
		if (font->glyf) {
			font->GSUB = otfcc_readTableotl(packet, options, 'GSUB');
			font->GPOS = otfcc_readTableotl(packet, options, 'GPOS');
			font->GDEF = otfcc_readTableGDEF(packet, options);
		}
		font->BASE = otfcc_readTableBASE(packet, options);
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
