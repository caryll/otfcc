#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caryll-font.h"
#include "caryll-sfnt-builder.h"
#include "caryll-sfnt.h"

caryll_font *caryll_new_font() {
	caryll_font *font = calloc(1, sizeof(caryll_font));
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
	font->VORG = NULL;
	return font;
}

void caryll_delete_font(caryll_font *font) {
	if (font->head) free(font->head);
	if (font->hhea) free(font->hhea);
	if (font->maxp) free(font->maxp);
	if (font->OS_2) free(font->OS_2);
	if (font->name) caryll_delete_name(font->name);
	if (font->hmtx) caryll_delete_hmtx(font->hmtx);
	if (font->post) caryll_delete_post(font->post);
	if (font->hdmx) caryll_delete_hdmx(font->hdmx);
	if (font->vhea) free(font->vhea);
	if (font->fpgm) caryll_delete_fpgm_prep(font->fpgm);
	if (font->prep) caryll_delete_fpgm_prep(font->prep);
	if (font->cvt_) caryll_delete_cvt(font->cvt_);
	if (font->gasp) caryll_delete_gasp(font->gasp);
	if (font->CFF_) caryll_delete_CFF(font->CFF_);
	if (font->glyf) caryll_delete_glyf(font->glyf);
	if (font->cmap) caryll_delete_cmap(font->cmap);
	if (font->GSUB) caryll_delete_otl(font->GSUB);
	if (font->GPOS) caryll_delete_otl(font->GPOS);
	if (font->GDEF) caryll_delete_GDEF(font->GDEF);
	if (font->VORG) caryll_delete_VORG(font->VORG);
	if (font->glyph_order && *font->glyph_order) { delete_glyph_order_map(font->glyph_order); }
	if (font) free(font);
}

caryll_font_subtype caryll_decide_font_subtype(caryll_sfnt *sfnt, uint32_t index) {
	caryll_packet packet = sfnt->packets[index];
	FOR_TABLE('CFF ', table) { return FONTTYPE_CFF; }
	return FONTTYPE_TTF;
}

caryll_font *caryll_read_font(caryll_sfnt *sfnt, uint32_t index) {
	if (sfnt->count - 1 < index)
		return NULL;
	else {
		caryll_font *font = caryll_new_font();
		caryll_packet packet = sfnt->packets[index];
		font->subtype = caryll_decide_font_subtype(sfnt, index);
		font->head = caryll_read_head(packet);
		font->maxp = caryll_read_maxp(packet);
		font->name = caryll_read_name(packet);
		font->OS_2 = caryll_read_OS_2(packet);
		font->post = caryll_read_post(packet);
		font->hhea = caryll_read_hhea(packet);
		font->cmap = caryll_read_cmap(packet);
		if (font->subtype == FONTTYPE_TTF) {
			font->hmtx = caryll_read_hmtx(packet, font->hhea, font->maxp);
			font->vhea = caryll_read_vhea(packet);
			if (font->vhea) font->vmtx = caryll_read_vmtx(packet, font->vhea, font->maxp);
			font->fpgm = caryll_read_fpgm_prep(packet, 'fpgm');
			font->prep = caryll_read_fpgm_prep(packet, 'prep');
			font->cvt_ = caryll_read_cvt(packet, 'cvt ');
			font->gasp = caryll_read_gasp(packet);
			font->glyf = caryll_read_glyf(packet, font->head, font->maxp);
		} else {
			caryll_cff_parse_result cffpr = caryll_read_CFF_and_glyf(packet);
			font->CFF_ = cffpr.meta;
			font->glyf = cffpr.glyphs;
			font->vhea = caryll_read_vhea(packet);
			if (font->vhea) {
				font->vmtx = caryll_read_vmtx(packet, font->vhea, font->maxp);
				font->VORG = caryll_read_VORG(packet);
			}
		}
		if (font->glyf) {
			font->GSUB = caryll_read_otl(packet, 'GSUB');
			font->GPOS = caryll_read_otl(packet, 'GPOS');
			font->GDEF = caryll_read_GDEF(packet);
		}
		return font;
	}
}

json_value *caryll_font_to_json(caryll_font *font, caryll_dump_options *dumpopts) {
	json_value *root = json_object_new(48);
	dumpopts->has_vertical_metrics = !!(font->vhea) && !!(font->vmtx);
	dumpopts->export_fdselect = font->CFF_ && font->CFF_->isCID;
	if (!root) return NULL;
	caryll_head_to_json(font->head, root, dumpopts);
	caryll_hhea_to_json(font->hhea, root, dumpopts);
	caryll_maxp_to_json(font->maxp, root, dumpopts);
	caryll_vhea_to_json(font->vhea, root, dumpopts);
	caryll_post_to_json(font->post, root, dumpopts);
	caryll_OS_2_to_json(font->OS_2, root, dumpopts);
	caryll_name_to_json(font->name, root, dumpopts);
	caryll_cmap_to_json(font->cmap, root, dumpopts);
	caryll_CFF_to_json(font->CFF_, root, dumpopts);
	caryll_glyf_to_json(font->glyf, root, dumpopts);
	if (!dumpopts->ignore_hints) {
		caryll_fpgm_prep_to_json(font->fpgm, root, dumpopts, "fpgm");
		caryll_fpgm_prep_to_json(font->prep, root, dumpopts, "prep");
		caryll_cvt_to_json(font->cvt_, root, dumpopts, "cvt_");
		caryll_gasp_to_json(font->gasp, root, dumpopts);
	}
	caryll_otl_to_json(font->GSUB, root, dumpopts, "GSUB");
	caryll_otl_to_json(font->GPOS, root, dumpopts, "GPOS");

	caryll_GDEF_to_json(font->GDEF, root, dumpopts);
	return root;
}

caryll_font *caryll_font_from_json(json_value *root, caryll_dump_options *dumpopts) {
	caryll_font *font = caryll_new_font();
	if (!font) return NULL;
	font->subtype = json_obj_get_type(root, "CFF_", json_object) != NULL;
	font->glyph_order = caryll_glyphorder_from_json(root, dumpopts);
	font->head = caryll_head_from_json(root, dumpopts);
	font->hhea = caryll_hhea_from_json(root, dumpopts);
	font->OS_2 = caryll_OS_2_from_json(root, dumpopts);
	font->maxp = caryll_maxp_from_json(root, dumpopts);
	font->post = caryll_post_from_json(root, dumpopts);
	font->name = caryll_name_from_json(root, dumpopts);
	font->cmap = caryll_cmap_from_json(root, dumpopts);
	font->CFF_ = caryll_CFF_from_json(root, dumpopts);
	font->glyf = caryll_glyf_from_json(root, *font->glyph_order, dumpopts);
	if (!dumpopts->ignore_hints) {
		font->fpgm = caryll_fpgm_prep_from_json(root, "fpgm");
		font->prep = caryll_fpgm_prep_from_json(root, "prep");
		font->cvt_ = caryll_cvt_from_json(root, "cvt_");
		font->gasp = caryll_gasp_from_json(root);
	}
	font->vhea = caryll_vhea_from_json(root, dumpopts);
	if (font->glyf) {
		font->GSUB = caryll_otl_from_json(root, dumpopts, "GSUB");
		font->GPOS = caryll_otl_from_json(root, dumpopts, "GPOS");
		font->GDEF = caryll_GDEF_from_json(root, dumpopts);
	}
	return font;
}

caryll_buffer *caryll_write_font(caryll_font *font, caryll_dump_options *dumpopts) {
	sfnt_builder *builder = new_sfnt_builder(font->subtype == FONTTYPE_CFF ? 'OTTO' : 0x00010000);

	// Outline data
	if (font->subtype == FONTTYPE_TTF) {
		caryll_buffer *bufglyf = bufnew();
		caryll_buffer *bufloca = bufnew();
		if (font->glyf && font->head) {
			caryll_write_glyf(font->glyf, font->head, bufglyf, bufloca, dumpopts);
		}
		sfnt_builder_push_table(builder, 'loca', bufloca);
		sfnt_builder_push_table(builder, 'glyf', bufglyf);
	} else {
		caryll_cff_parse_result r = {font->CFF_, font->glyf};
		sfnt_builder_push_table(builder, 'CFF ', caryll_write_CFF(r, dumpopts));
	}

	sfnt_builder_push_table(builder, 'head', caryll_write_head(font->head, dumpopts));
	sfnt_builder_push_table(builder, 'hhea', caryll_write_hhea(font->hhea, dumpopts));
	sfnt_builder_push_table(builder, 'OS/2', caryll_write_OS_2(font->OS_2, dumpopts));
	sfnt_builder_push_table(builder, 'maxp', caryll_write_maxp(font->maxp, dumpopts));
	sfnt_builder_push_table(builder, 'name', caryll_write_name(font->name, dumpopts));
	sfnt_builder_push_table(builder, 'post',
	                        caryll_write_post(font->post, font->glyph_order, dumpopts));
	sfnt_builder_push_table(builder, 'cmap', caryll_write_cmap(font->cmap, dumpopts));
	if (font->fpgm)
		sfnt_builder_push_table(builder, 'fpgm', caryll_write_fpgm_prep(font->fpgm, dumpopts));
	if (font->prep)
		sfnt_builder_push_table(builder, 'prep', caryll_write_fpgm_prep(font->prep, dumpopts));
	if (font->cvt_)
		sfnt_builder_push_table(builder, 'cvt ', caryll_write_cvt(font->cvt_, dumpopts));
	if (font->gasp)
		sfnt_builder_push_table(builder, 'gasp', caryll_write_gasp(font->gasp, dumpopts));

	sfnt_builder_push_table(builder, 'hmtx',
	                        caryll_write_hmtx(font->hmtx, font->hhea->numberOfMetrics,
	                                          font->maxp->numGlyphs - font->hhea->numberOfMetrics,
	                                          dumpopts));

	if (font->vhea)
		sfnt_builder_push_table(builder, 'vhea', caryll_write_vhea(font->vhea, dumpopts));
	if (font->vmtx) {
		sfnt_builder_push_table(
		    builder, 'vmtx',
		    caryll_write_vmtx(font->vmtx, font->vhea->numOfLongVerMetrics,
		                      font->maxp->numGlyphs - font->vhea->numOfLongVerMetrics, dumpopts));
	}

	if (font->GSUB)
		sfnt_builder_push_table(builder, 'GSUB', caryll_write_otl(font->GSUB, dumpopts));
	if (font->GPOS)
		sfnt_builder_push_table(builder, 'GPOS', caryll_write_otl(font->GPOS, dumpopts));
	if (font->GDEF)
		sfnt_builder_push_table(builder, 'GDEF', caryll_write_GDEF(font->GDEF, dumpopts));

	if (dumpopts->dummy_DSIG) {
		caryll_buffer *dsig = bufnew();
		bufwrite32b(dsig, 0x00000001);
		bufwrite16b(dsig, 0);
		bufwrite16b(dsig, 0);
		sfnt_builder_push_table(builder, 'DSIG', dsig);
	}

	caryll_buffer *otf = sfnt_builder_serialize(builder);
	delete_sfnt_builder(builder);
	return otf;
}
