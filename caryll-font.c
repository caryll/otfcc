#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caryll-sfnt.h"
#include "caryll-font.h"
#include "caryll-sfnt-builder.h"

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
	return font;
}

void caryll_delete_font(caryll_font *font) {
	if (font->glyf != NULL) caryll_delete_glyf(font->glyf);
	if (font->cmap != NULL) caryll_delete_cmap(font->cmap);
	if (font->head != NULL) free(font->head);
	if (font->hhea != NULL) free(font->hhea);
	if (font->maxp != NULL) free(font->maxp);
	if (font->OS_2 != NULL) free(font->OS_2);
	if (font->name != NULL) caryll_delete_name(font->name);
	if (font->hmtx != NULL) caryll_delete_hmtx(font->hmtx);
	if (font->post != NULL) caryll_delete_post(font->post);
	if (font->hdmx != NULL) caryll_delete_hdmx(font->hdmx);
	if (font->fpgm != NULL) caryll_delete_fpgm_prep(font->fpgm);
	if (font->prep != NULL) caryll_delete_fpgm_prep(font->prep);
	if (font->cvt_ != NULL) caryll_delete_fpgm_prep(font->cvt_);
	if (font->gasp != NULL) caryll_delete_gasp(font->gasp);
	if (font->glyph_order && *font->glyph_order) { delete_glyph_order_map(font->glyph_order); }
	if (font != NULL) free(font);
}

caryll_font *caryll_read_font(caryll_sfnt *sfnt, uint32_t index) {
	if (sfnt->count - 1 < index)
		return NULL;
	else {
		caryll_font *font = caryll_new_font();
		caryll_packet packet = sfnt->packets[index];

		font->head = caryll_read_head(packet);
		font->maxp = caryll_read_maxp(packet);
		font->hhea = caryll_read_hhea(packet);
		font->name = caryll_read_name(packet);
		font->OS_2 = caryll_read_OS_2(packet);
		font->hmtx = caryll_read_hmtx(packet, font->hhea, font->maxp);
		font->post = caryll_read_post(packet);
		font->hdmx = caryll_read_hdmx(packet, font->maxp);
		font->glyf = caryll_read_glyf(packet, font->head, font->maxp);
		font->cmap = caryll_read_cmap(packet);
		font->fpgm = caryll_read_fpgm_prep(packet, 'fpgm');
		font->prep = caryll_read_fpgm_prep(packet, 'prep');
		font->cvt_ = caryll_read_fpgm_prep(packet, 'cvt ');
		font->gasp = caryll_read_gasp(packet);

		caryll_font_unconsolidate(font);

		return font;
	}
}

json_value *caryll_font_to_json(caryll_font *font, caryll_dump_options dumpopts) {
	json_value *root = json_object_new(12);
	if (!root) return NULL;
	caryll_head_to_json(font->head, root, dumpopts);
	caryll_hhea_to_json(font->hhea, root, dumpopts);
	caryll_maxp_to_json(font->maxp, root, dumpopts);
	caryll_name_to_json(font->name, root, dumpopts);
	caryll_post_to_json(font->post, root, dumpopts);
	caryll_OS_2_to_json(font->OS_2, root, dumpopts);
	caryll_cmap_to_json(font->cmap, root, dumpopts);
	caryll_glyf_to_json(font->glyf, root, dumpopts);
	if (!dumpopts.ignore_hints) {
		caryll_fpgm_prep_to_json(font->fpgm, root, dumpopts, "fpgm");
		caryll_fpgm_prep_to_json(font->prep, root, dumpopts, "prep");
		caryll_fpgm_prep_to_json(font->cvt_, root, dumpopts, "cvt_");
		caryll_gasp_to_json(font->gasp, root, dumpopts);
	}
	return root;
}

caryll_font *caryll_font_from_json(json_value *root, caryll_dump_options dumpopts) {
	caryll_font *font = caryll_new_font();
	if (!font) return NULL;
	font->glyph_order = caryll_glyphorder_from_json(root, dumpopts);
	font->head = caryll_head_from_json(root, dumpopts);
	font->hhea = caryll_hhea_from_json(root, dumpopts);
	font->OS_2 = caryll_OS_2_from_json(root, dumpopts);
	font->maxp = caryll_maxp_from_json(root, dumpopts);
	font->post = caryll_post_from_json(root, dumpopts);
	font->name = caryll_name_from_json(root, dumpopts);
	font->cmap = caryll_cmap_from_json(root, dumpopts);
	font->glyf = caryll_glyf_from_json(root, *font->glyph_order, dumpopts);
	if (!dumpopts.ignore_hints) {
		font->fpgm = caryll_fpgm_prep_from_json(root, "fpgm");
		font->prep = caryll_fpgm_prep_from_json(root, "prep");
		font->cvt_ = caryll_fpgm_prep_from_json(root, "cvt_");
		font->gasp = caryll_gasp_from_json(root);
	}
	return font;
}
caryll_buffer *caryll_write_font(caryll_font *font) {
	caryll_buffer *bufglyf = bufnew();
	caryll_buffer *bufloca = bufnew();
	caryll_write_glyf(font->glyf, font->head, bufglyf, bufloca);

	sfnt_builder *builder = new_sfnt_builder();
	sfnt_builder_push_table(builder, 'head', caryll_write_head(font->head));
	sfnt_builder_push_table(builder, 'hhea', caryll_write_hhea(font->hhea));
	sfnt_builder_push_table(builder, 'OS/2', caryll_write_OS_2(font->OS_2));
	sfnt_builder_push_table(builder, 'maxp', caryll_write_maxp(font->maxp));
	sfnt_builder_push_table(builder, 'name', caryll_write_name(font->name));
	sfnt_builder_push_table(builder, 'post', caryll_write_post(font->post));
	sfnt_builder_push_table(builder, 'hmtx', caryll_write_hmtx(font->hmtx, font->hhea->numberOfMetrics,
	                                                           font->maxp->numGlyphs - font->hhea->numberOfMetrics));
	sfnt_builder_push_table(builder, 'loca', bufloca);
	sfnt_builder_push_table(builder, 'glyf', bufglyf);
	sfnt_builder_push_table(builder, 'cmap', caryll_write_cmap(font->cmap));
	if (font->fpgm) sfnt_builder_push_table(builder, 'fpgm', caryll_write_fpgm_prep(font->fpgm));
	if (font->prep) sfnt_builder_push_table(builder, 'prep', caryll_write_fpgm_prep(font->prep));
	if (font->cvt_) sfnt_builder_push_table(builder, 'cvt ', caryll_write_fpgm_prep(font->cvt_));
	if (font->gasp) sfnt_builder_push_table(builder, 'gasp', caryll_write_gasp(font->gasp));
	
	caryll_buffer *otf = sfnt_builder_serialize(builder);
	delete_sfnt_builder(builder);
	return otf;
}
