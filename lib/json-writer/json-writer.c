#include "support/util.h"
#include "otfcc/font.h"

static void *serializeToJson(otfcc_Font *font, const otfcc_Options *options) {
	json_value *root = json_object_new(48);
	if (!root) return NULL;
	otfcc_dumpHead(font->head, root, options);
	otfcc_dumpHhea(font->hhea, root, options);
	otfcc_dumpMaxp(font->maxp, root, options);
	otfcc_dumpVhea(font->vhea, root, options);
	otfcc_dumpPost(font->post, root, options);
	otfcc_dumpOS_2(font->OS_2, root, options);
	otfcc_dumpName(font->name, root, options);
	otfcc_dumpCmap(font->cmap, root, options);
	otfcc_dumpCFF(font->CFF_, root, options);
	otfcc_dumpGlyf(font->glyf, root, options,        //
	               !!(font->vhea) && !!(font->vmtx), // whether export vertical metrics
	               font->CFF_ && font->CFF_->isCID   // whether export FDSelect
	               );
	if (!options->ignore_hints) {
		table_dumpTableFpgmPrep(font->fpgm, root, options, "fpgm");
		table_dumpTableFpgmPrep(font->prep, root, options, "prep");
		otfcc_dumpCvt(font->cvt_, root, options, "cvt_");
		otfcc_dumpGasp(font->gasp, root, options);
	}
	otfcc_dumpOtl(font->GSUB, root, options, "GSUB");
	otfcc_dumpOtl(font->GPOS, root, options, "GPOS");
	otfcc_dumpGDEF(font->GDEF, root, options);
	otfcc_dumpBASE(font->BASE, root, options);
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
