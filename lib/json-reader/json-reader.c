#include "support/util.h"
#include "otfcc/font.h"

static caryll_font_subtype caryll_decideFontSubtypeFromJson(json_value *root) {
	if (json_obj_get_type(root, "CFF_", json_object) != NULL) {
		return FONTTYPE_CFF;
	} else {
		return FONTTYPE_TTF;
	}
}
static caryll_Font *readJson(void *_root, uint32_t index, const otfcc_Options *options) {
	json_value *root = (json_value *)_root;
	caryll_Font *font = caryll_new_Font();
	if (!font) return NULL;
	font->subtype = caryll_decideFontSubtypeFromJson(root);
	font->glyph_order = caryll_parse_GlyphOrder(root, options);
	font->glyf = table_parse_glyf(root, font->glyph_order, options);
	font->CFF_ = table_parse_cff(root, options);
	font->head = table_parse_head(root, options);
	font->hhea = table_parse_hhea(root, options);
	font->OS_2 = table_parse_OS_2(root, options);
	font->maxp = table_parse_maxp(root, options);
	font->post = table_parse_post(root, options);
	font->name = table_parse_name(root, options);
	font->cmap = table_parse_cmap(root, options);
	if (!options->ignore_hints) {
		font->fpgm = table_fpgm_parse_prep(root, options, "fpgm");
		font->prep = table_fpgm_parse_prep(root, options, "prep");
		font->cvt_ = table_parse_cvt(root, options, "cvt_");
		font->gasp = table_parse_gasp(root, options);
	}
	font->vhea = table_parse_vhea(root, options);
	if (font->glyf) {
		font->GSUB = table_parse_otl(root, options, "GSUB");
		font->GPOS = table_parse_otl(root, options, "GPOS");
		font->GDEF = table_parse_GDEF(root, options);
	}
	font->BASE = table_parse_BASE(root, options);
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
