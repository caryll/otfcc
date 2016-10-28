#include "support/util.h"
#include "otfcc/font.h"

static void *serializeToJson(caryll_Font *font, const otfcc_Options *options) {
	json_value *root = json_object_new(48);
	if (!root) return NULL;
	table_dump_head(font->head, root, options);
	table_dump_hhea(font->hhea, root, options);
	table_dump_maxp(font->maxp, root, options);
	table_dump_vhea(font->vhea, root, options);
	table_dump_post(font->post, root, options);
	table_dump_OS_2(font->OS_2, root, options);
	table_dump_name(font->name, root, options);
	table_dump_cmap(font->cmap, root, options);
	table_dump_cff(font->CFF_, root, options);
	table_dump_glyf(font->glyf, root, options,        //
	                !!(font->vhea) && !!(font->vmtx), // whether export vertical metrics
	                font->CFF_ && font->CFF_->isCID   // whether export FDSelect
	                );
	if (!options->ignore_hints) {
		table_fpgm_dump_prep(font->fpgm, root, options, "fpgm");
		table_fpgm_dump_prep(font->prep, root, options, "prep");
		table_dump_cvt(font->cvt_, root, options, "cvt_");
		table_dump_gasp(font->gasp, root, options);
	}
	table_dump_otl(font->GSUB, root, options, "GSUB");
	table_dump_otl(font->GPOS, root, options, "GPOS");
	table_dump_GDEF(font->GDEF, root, options);
	table_dump_BASE(font->BASE, root, options);
	return root;
}
static void dispose(otfcc_IFontSerializer *self) {
	free(self);
}
otfcc_IFontSerializer *otfcc_newJsonWriter() {
	otfcc_IFontSerializer *writer;
	NEW(writer);
	writer->serialize = serializeToJson;
	writer->dispose = dispose;
	return writer;
}
