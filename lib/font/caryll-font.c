#include "support/util.h"
#include "otfcc/font.h"
#include "otfcc/sfnt-builder.h"

caryll_Font *caryll_new_Font() {
	caryll_Font *font = calloc(1, sizeof(caryll_Font));
	if (!font) return NULL;
	font->head = NULL;
	font->hhea = NULL;
	font->maxp = NULL;
	font->hmtx = NULL;
	font->post = NULL;
	font->name = NULL;
	font->hdmx = NULL;
	font->glyf = NULL;
	font->cmap = NULL;
	font->glyph_order = NULL;
	font->fpgm = NULL;
	font->prep = NULL;
	font->cvt_ = NULL;
	font->gasp = NULL;
	font->vhea = NULL;
	font->vmtx = NULL;
	font->GSUB = NULL;
	font->GPOS = NULL;
	font->GDEF = NULL;
	font->BASE = NULL;
	font->VORG = NULL;
	return font;
}

void caryll_delete_Font(caryll_Font *font) {
	if (font->head) free(font->head);
	if (font->hhea) free(font->hhea);
	if (font->maxp) table_delete_maxp(font->maxp);
	if (font->OS_2) free(font->OS_2);
	if (font->name) table_delete_name(font->name);
	if (font->hmtx) table_delete_hmtx(font->hmtx);
	if (font->post) table_delete_post(font->post);
	if (font->hdmx) table_delete_hdmx(font->hdmx);
	if (font->vhea) free(font->vhea);
	if (font->fpgm) table_delete_fpgm_prep(font->fpgm);
	if (font->prep) table_delete_fpgm_prep(font->prep);
	if (font->cvt_) table_delete_cvt(font->cvt_);
	if (font->gasp) table_delete_gasp(font->gasp);
	if (font->CFF_) table_delete_CFF(font->CFF_);
	if (font->glyf) table_delete_glyf(font->glyf);
	if (font->cmap) table_delete_cmap(font->cmap);
	if (font->LTSH) table_delete_LTSH(font->LTSH);
	if (font->GSUB) table_delete_otl(font->GSUB);
	if (font->GPOS) table_delete_otl(font->GPOS);
	if (font->GDEF) table_delete_GDEF(font->GDEF);
	if (font->BASE) table_delete_BASE(font->BASE);
	if (font->VORG) table_delete_VORG(font->VORG);
	if (font->glyph_order) { caryll_delete_GlyphOrder(font->glyph_order); }
	if (font) free(font);
}

static caryll_font_subtype caryll_decideFontSubtype(caryll_SplineFontContainer *sfnt, uint32_t index) {
	caryll_Packet packet = sfnt->packets[index];
	FOR_TABLE('CFF ', table) {
		return FONTTYPE_CFF;
	}
	return FONTTYPE_TTF;
}

caryll_Font *caryll_read_Font(caryll_SplineFontContainer *sfnt, uint32_t index, otfcc_Options *options) {
	if (sfnt->count - 1 < index) {
		return NULL;
	} else {
		caryll_Font *font = caryll_new_Font();
		caryll_Packet packet = sfnt->packets[index];
		font->subtype = caryll_decideFontSubtype(sfnt, index);
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
		return font;
	}
}

json_value *caryll_dump_Font(caryll_Font *font, otfcc_Options *options) {
	json_value *root = json_object_new(48);
	options->has_vertical_metrics = !!(font->vhea) && !!(font->vmtx);
	options->export_fdselect = font->CFF_ && font->CFF_->isCID;
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
	table_dump_glyf(font->glyf, root, options);
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

static caryll_font_subtype caryll_decideFontSubtypeFromJson(json_value *root) {
	if (json_obj_get_type(root, "CFF_", json_object) != NULL) {
		return FONTTYPE_CFF;
	} else {
		return FONTTYPE_TTF;
	}
}
caryll_Font *caryll_parse_Font(json_value *root, otfcc_Options *options) {
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

caryll_Buffer *caryll_build_Font(caryll_Font *font, otfcc_Options *options) {
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
	return otf;
}
