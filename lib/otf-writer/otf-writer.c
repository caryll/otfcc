#include "support/util.h"
#include "otfcc/font.h"
#include "otfcc/sfnt-builder.h"
#include "stat.h"

static void *serializeToOTF(caryll_Font *font, const otfcc_Options *options) {
	// do stat before serialize
	otfcc_statFont(font, options);

	caryll_SFNTBuilder *builder = caryll_new_SFNTBuilder(font->subtype == FONTTYPE_CFF ? 'OTTO' : 0x00010000, options);
	// Outline data
	if (font->subtype == FONTTYPE_TTF) {
		glyf_loca_bufpair pair = table_build_glyf(font->glyf, font->head, options);
		caryll_pushTableToSfntBuilder(builder, 'glyf', pair.glyf);
		caryll_pushTableToSfntBuilder(builder, 'loca', pair.loca);
	} else {
		table_CFFAndGlyf r = {font->CFF_, font->glyf};
		caryll_pushTableToSfntBuilder(builder, 'CFF ', table_build_CFF(r, options));
	}

	caryll_pushTableToSfntBuilder(builder, 'head', table_build_head(font->head, options));
	caryll_pushTableToSfntBuilder(builder, 'hhea', table_build_hhea(font->hhea, options));
	caryll_pushTableToSfntBuilder(builder, 'OS/2', table_build_OS_2(font->OS_2, options));
	caryll_pushTableToSfntBuilder(builder, 'maxp', table_build_maxp(font->maxp, options));
	caryll_pushTableToSfntBuilder(builder, 'name', table_build_name(font->name, options));
	caryll_pushTableToSfntBuilder(builder, 'post', table_build_post(font->post, font->glyph_order, options));
	caryll_pushTableToSfntBuilder(builder, 'cmap', table_build_cmap(font->cmap, options));
	if (font->gasp) caryll_pushTableToSfntBuilder(builder, 'gasp', table_build_gasp(font->gasp, options));

	if (font->subtype == FONTTYPE_TTF) {
		if (font->fpgm) caryll_pushTableToSfntBuilder(builder, 'fpgm', table_build_fpgm_prep(font->fpgm, options));
		if (font->prep) caryll_pushTableToSfntBuilder(builder, 'prep', table_build_fpgm_prep(font->prep, options));
		if (font->cvt_) caryll_pushTableToSfntBuilder(builder, 'cvt ', table_build_cvt(font->cvt_, options));
		if (font->LTSH) caryll_pushTableToSfntBuilder(builder, 'LTSH', table_build_LTSH(font->LTSH, options));
	}

	if (font->hhea && font->maxp && font->hmtx) {
		uint16_t hmtx_counta = font->hhea->numberOfMetrics;
		uint16_t hmtx_countk = font->maxp->numGlyphs - font->hhea->numberOfMetrics;
		caryll_pushTableToSfntBuilder(builder, 'hmtx', table_build_hmtx(font->hmtx, hmtx_counta, hmtx_countk, options));
	}
	if (font->vhea) caryll_pushTableToSfntBuilder(builder, 'vhea', table_build_vhea(font->vhea, options));
	if (font->vhea && font->maxp && font->vmtx) {
		uint16_t vmtx_counta = font->vhea->numOfLongVerMetrics;
		uint16_t vmtx_countk = font->maxp->numGlyphs - font->vhea->numOfLongVerMetrics;
		caryll_pushTableToSfntBuilder(builder, 'vmtx', table_build_vmtx(font->vmtx, vmtx_counta, vmtx_countk, options));
	}
	if (font->VORG) { caryll_pushTableToSfntBuilder(builder, 'VORG', table_build_VORG(font->VORG, options)); }

	if (font->GSUB) caryll_pushTableToSfntBuilder(builder, 'GSUB', table_build_otl(font->GSUB, options, "GSUB"));
	if (font->GPOS) caryll_pushTableToSfntBuilder(builder, 'GPOS', table_build_otl(font->GPOS, options, "GPOS"));
	if (font->GDEF) caryll_pushTableToSfntBuilder(builder, 'GDEF', table_build_GDEF(font->GDEF, options));
	if (font->BASE) caryll_pushTableToSfntBuilder(builder, 'BASE', table_build_BASE(font->BASE, options));

	if (options->dummy_DSIG) {
		caryll_Buffer *dsig = bufnew();
		bufwrite32b(dsig, 0x00000001);
		bufwrite16b(dsig, 0);
		bufwrite16b(dsig, 0);
		caryll_pushTableToSfntBuilder(builder, 'DSIG', dsig);
	}

	caryll_Buffer *otf = caryll_serializeSFNT(builder);
	caryll_delete_SFNTBuilder(builder);
	otfcc_unstatFont(font, options);
	return otf;
}

otfcc_IFontSerializer *otfcc_newOTFWriter() {
	otfcc_IFontSerializer *writer;
	NEW(writer);
	writer->serialize = serializeToOTF;
	return writer;
}
