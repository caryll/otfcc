#include "caryll-sfnt.h"
#include "caryll-font.h"
#include "extern/json.h"
#include "support/stopwatch.h"
#include "caryll-sfnt-builder.h"

void print_table(sfnt_builder_entry *t) {
	fprintf(stderr, "Writing Table %c%c%c%c, Length: %8d, Checksum: %08X\n", ((uint32_t)(t->tag) >> 24) & 0xff,
	        ((uint32_t)(t->tag) >> 16) & 0xff, ((uint32_t)(t->tag) >> 8) & 0xff, t->tag & 0xff, t->length, t->checksum);
}

int main(int argc, char *argv[]) {
	struct timespec begin;
	time_now(&begin);

	char *buffer = 0;
	long length;
	FILE *f = fopen(argv[1], "rb");

	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer) { fread(buffer, 1, length, f); }
		fclose(f);
	}
	push_stopwatch("Read file", &begin);
	if (buffer) {
		json_value *root = json_parse(buffer, length);
		free(buffer);
		push_stopwatch("Parse JSON", &begin);
		if (root) {
			caryll_font *font = caryll_font_new();
			font->head = caryll_head_from_json(root);
			font->hhea = caryll_hhea_from_json(root);
			font->OS_2 = caryll_OS_2_from_json(root);
			font->maxp = caryll_maxp_from_json(root);
			font->post = caryll_post_from_json(root);
			font->glyph_order = caryll_glyphorder_from_json(root);
			font->cmap = caryll_cmap_from_json(root);
			font->glyf = caryll_glyf_from_json(root, *font->glyph_order);
			push_stopwatch("Parse json to font", &begin);

			json_value_free(root);
			caryll_font_consolidate(font);
			push_stopwatch("Consolidation", &begin);

			caryll_font_stat(font);
			push_stopwatch("Stating", &begin);
			{
				caryll_buffer *bufglyf = bufnew();
				caryll_buffer *bufloca = bufnew();
				caryll_write_glyf(font->glyf, font->head, bufglyf, bufloca);

				sfnt_builder *builder = sfnt_builder_new();
				sfnt_builder_push_table(builder, 'head', caryll_write_head(font->head));
				sfnt_builder_push_table(builder, 'hhea', caryll_write_hhea(font->hhea));
				sfnt_builder_push_table(builder, 'OS/2', caryll_write_OS_2(font->OS_2));
				sfnt_builder_push_table(builder, 'maxp', caryll_write_maxp(font->maxp));
				sfnt_builder_push_table(builder, 'post', caryll_write_post(font->post));
				sfnt_builder_push_table(builder, 'loca', bufloca);
				sfnt_builder_push_table(builder, 'glyf', bufglyf);

				caryll_buffer *otf = sfnt_builder_serialize(builder);
				FILE *outfile = fopen(argv[2], "wb");
				fwrite(otf->s, sizeof(uint8_t), buflen(otf), outfile);
				fclose(outfile);

				sfnt_builder_delete(builder);
				buffree(otf);
			}

			caryll_font_close(font);
			push_stopwatch("Finalize", &begin);
		}
	}
	return 0;
}
