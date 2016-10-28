#include "support/util.h"
#include "otfcc/font.h"
#include "otfcc/sfnt-builder.h"
#include "stat.h"

static void *serializeToOTF(otfcc_Font *font, const otfcc_Options *options) {
	// do stat before serialize
	otfcc_statFont(font, options);

	otfcc_SFNTBuilder *builder = otfcc_newSFNTBuilder(font->subtype == FONTTYPE_CFF ? 'OTTO' : 0x00010000, options);
	// Outline data
	if (font->subtype == FONTTYPE_TTF) {
		glyf_loca_bufpair pair = otfcc_buildTableglyf(font->glyf, font->head, options);
		otfcc_SFNTBuilder_pushTable(builder, 'glyf', pair.glyf);
		otfcc_SFNTBuilder_pushTable(builder, 'loca', pair.loca);
	} else {
		table_CFFAndGlyf r = {font->CFF_, font->glyf};
		otfcc_SFNTBuilder_pushTable(builder, 'CFF ', otfcc_buildTableCFF(r, options));
	}

	otfcc_SFNTBuilder_pushTable(builder, 'head', otfcc_buildTablehead(font->head, options));
	otfcc_SFNTBuilder_pushTable(builder, 'hhea', otfcc_buildTablehhea(font->hhea, options));
	otfcc_SFNTBuilder_pushTable(builder, 'OS/2', otfcc_buildTableOS_2(font->OS_2, options));
	otfcc_SFNTBuilder_pushTable(builder, 'maxp', otfcc_buildTablemaxp(font->maxp, options));
	otfcc_SFNTBuilder_pushTable(builder, 'name', otfcc_buildTablename(font->name, options));
	otfcc_SFNTBuilder_pushTable(builder, 'post', otfcc_buildTablepost(font->post, font->glyph_order, options));
	otfcc_SFNTBuilder_pushTable(builder, 'cmap', otfcc_buildTablecmap(font->cmap, options));
	if (font->gasp) otfcc_SFNTBuilder_pushTable(builder, 'gasp', otfcc_buildTablegasp(font->gasp, options));

	if (font->subtype == FONTTYPE_TTF) {
		if (font->fpgm) otfcc_SFNTBuilder_pushTable(builder, 'fpgm', otfcc_buildTablefpgm_prep(font->fpgm, options));
		if (font->prep) otfcc_SFNTBuilder_pushTable(builder, 'prep', otfcc_buildTablefpgm_prep(font->prep, options));
		if (font->cvt_) otfcc_SFNTBuilder_pushTable(builder, 'cvt ', otfcc_buildTablecvt(font->cvt_, options));
		if (font->LTSH) otfcc_SFNTBuilder_pushTable(builder, 'LTSH', otfcc_buildTableLTSH(font->LTSH, options));
	}

	if (font->hhea && font->maxp && font->hmtx) {
		uint16_t hmtx_counta = font->hhea->numberOfMetrics;
		uint16_t hmtx_countk = font->maxp->numGlyphs - font->hhea->numberOfMetrics;
		otfcc_SFNTBuilder_pushTable(builder, 'hmtx',
		                            otfcc_buildTablehmtx(font->hmtx, hmtx_counta, hmtx_countk, options));
	}
	if (font->vhea) otfcc_SFNTBuilder_pushTable(builder, 'vhea', otfcc_buildTablevhea(font->vhea, options));
	if (font->vhea && font->maxp && font->vmtx) {
		uint16_t vmtx_counta = font->vhea->numOfLongVerMetrics;
		uint16_t vmtx_countk = font->maxp->numGlyphs - font->vhea->numOfLongVerMetrics;
		otfcc_SFNTBuilder_pushTable(builder, 'vmtx',
		                            otfcc_buildTablevmtx(font->vmtx, vmtx_counta, vmtx_countk, options));
	}
	if (font->VORG) { otfcc_SFNTBuilder_pushTable(builder, 'VORG', otfcc_buildTableVORG(font->VORG, options)); }

	if (font->GSUB) otfcc_SFNTBuilder_pushTable(builder, 'GSUB', otfcc_buildTableotl(font->GSUB, options, "GSUB"));
	if (font->GPOS) otfcc_SFNTBuilder_pushTable(builder, 'GPOS', otfcc_buildTableotl(font->GPOS, options, "GPOS"));
	if (font->GDEF) otfcc_SFNTBuilder_pushTable(builder, 'GDEF', otfcc_buildTableGDEF(font->GDEF, options));
	if (font->BASE) otfcc_SFNTBuilder_pushTable(builder, 'BASE', otfcc_buildTableBASE(font->BASE, options));

	if (options->dummy_DSIG) {
		caryll_Buffer *dsig = bufnew();
		bufwrite32b(dsig, 0x00000001);
		bufwrite16b(dsig, 0);
		bufwrite16b(dsig, 0);
		otfcc_SFNTBuilder_pushTable(builder, 'DSIG', dsig);
	}

	caryll_Buffer *otf = otfcc_SFNTBuilder_serialize(builder);
	otfcc_delete_SFNTBuilder(builder);
	otfcc_unstatFont(font, options);
	return otf;
}
static void dispose(otfcc_IFontSerializer *self) {
	free(self);
}
otfcc_IFontSerializer *otfcc_newOTFWriter() {
	otfcc_IFontSerializer *writer;
	NEW(writer);
	writer->serialize = serializeToOTF;
	writer->dispose = dispose;
	return writer;
}
