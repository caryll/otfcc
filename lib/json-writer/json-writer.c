#include "support/util.h"
#include "otfcc/font.h"

static void *serializeToJson(otfcc_Font *font, const otfcc_Options *options) {
	json_value *root = json_object_new(48);
	if (!root) return NULL;
	otfcc_dumpTablehead(font->head, root, options);
	otfcc_dumpTablehhea(font->hhea, root, options);
	otfcc_dumpTablemaxp(font->maxp, root, options);
	otfcc_dumpTablevhea(font->vhea, root, options);
	otfcc_dumpTablepost(font->post, root, options);
	otfcc_dumpTableOS_2(font->OS_2, root, options);
	otfcc_dumpTablename(font->name, root, options);
	otfcc_dumpTablecmap(font->cmap, root, options);
	otfcc_dumpTablecff(font->CFF_, root, options);
	otfcc_dumpTableglyf(font->glyf, root, options,        //
	                !!(font->vhea) && !!(font->vmtx), // whether export vertical metrics
	                font->CFF_ && font->CFF_->isCID   // whether export FDSelect
	                );
	if (!options->ignore_hints) {
		table_fpgm_dump_prep(font->fpgm, root, options, "fpgm");
		table_fpgm_dump_prep(font->prep, root, options, "prep");
		otfcc_dumpTablecvt(font->cvt_, root, options, "cvt_");
		otfcc_dumpTablegasp(font->gasp, root, options);
	}
	otfcc_dumpTableotl(font->GSUB, root, options, "GSUB");
	otfcc_dumpTableotl(font->GPOS, root, options, "GPOS");
	otfcc_dumpTableGDEF(font->GDEF, root, options);
	otfcc_dumpTableBASE(font->BASE, root, options);
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
