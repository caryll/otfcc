#include "support/util.h"
#include "otfcc/font.h"

static otfcc_font_subtype otfcc_decideFontSubtypeFromJson(json_value *root) {
	if (json_obj_get_type(root, "CFF_", json_object) != NULL) {
		return FONTTYPE_CFF;
	} else {
		return FONTTYPE_TTF;
	}
}
static otfcc_Font *readJson(void *_root, uint32_t index, const otfcc_Options *options) {
	json_value *root = (json_value *)_root;
	otfcc_Font *font = otfcc_new_Font();
	if (!font) return NULL;
	font->subtype = otfcc_decideFontSubtypeFromJson(root);
	font->glyph_order = otfcc_parse_GlyphOrder(root, options);
	font->glyf = otfcc_parseTableglyf(root, font->glyph_order, options);
	font->CFF_ = otfcc_parseTablecff(root, options);
	font->head = otfcc_parseTablehead(root, options);
	font->hhea = otfcc_parseTablehhea(root, options);
	font->OS_2 = otfcc_parseTableOS_2(root, options);
	font->maxp = otfcc_parseTablemaxp(root, options);
	font->post = otfcc_parseTablepost(root, options);
	font->name = otfcc_parseTablename(root, options);
	font->cmap = otfcc_parseTablecmap(root, options);
	if (!options->ignore_hints) {
		font->fpgm = table_fpgm_parse_prep(root, options, "fpgm");
		font->prep = table_fpgm_parse_prep(root, options, "prep");
		font->cvt_ = otfcc_parseTablecvt(root, options, "cvt_");
		font->gasp = otfcc_parseTablegasp(root, options);
	}
	font->vhea = otfcc_parseTablevhea(root, options);
	if (font->glyf) {
		font->GSUB = otfcc_parseTableotl(root, options, "GSUB");
		font->GPOS = otfcc_parseTableotl(root, options, "GPOS");
		font->GDEF = otfcc_parseTableGDEF(root, options);
	}
	font->BASE = otfcc_parseTableBASE(root, options);
	return font;
}
static void disposeReader(otfcc_IFontBuilder *self) {
	free(self);
}
otfcc_IFontBuilder *otfcc_newJsonReader() {
	otfcc_IFontBuilder *reader;
	NEW(reader);
	reader->create = readJson;
	reader->dispose = disposeReader;
	return reader;
}
