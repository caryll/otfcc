#include "caryll-sfnt.h"
#include "caryll-font.h"
#include "extern/json.h"
#include "caryll-sfnt-builder.h"

#include <getopt.h>
#include "support/stopwatch.h"
#include "version.h"

void print_table(sfnt_builder_entry *t) {
	fprintf(stderr, "Writing Table %c%c%c%c, Length: %8d, Checksum: %08X\n", ((uint32_t)(t->tag) >> 24) & 0xff,
	        ((uint32_t)(t->tag) >> 16) & 0xff, ((uint32_t)(t->tag) >> 8) & 0xff, t->tag & 0xff, t->length, t->checksum);
}

int main(int argc, char *argv[]) {
	struct timespec begin;
	time_now(&begin);

	bool show_help = false;
	bool show_version = false;
	bool show_time = false;
	sds outputPath = NULL;
	sds inPath = NULL;
	int option_index = 0;
	int c;
	caryll_dump_options dumpopts = {.ignore_glyph_order = false, .ignore_hints = false};

	struct option longopts[] = {{"version", no_argument, NULL, 'v'},
	                            {"help", no_argument, NULL, 'h'},
	                            {"time", no_argument, NULL, 0},
	                            {"ignore-glyph-order", no_argument, NULL, 0},
	                            {"ignore-hints", no_argument, NULL, 0},
	                            {"output", required_argument, NULL, 'o'},
	                            {0, 0, 0, 0}};

	while ((c = getopt_long(argc, argv, "vhpo:n:", longopts, &option_index)) != (-1)) {
		switch (c) {
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (longopts[option_index].flag != 0) break;
			if (strcmp(longopts[option_index].name, "time") == 0) { show_time = true; }
			if (strcmp(longopts[option_index].name, "ignore-glyph-order") == 0) { dumpopts.ignore_glyph_order = true; }
			if (strcmp(longopts[option_index].name, "ignore-hints") == 0) { dumpopts.ignore_hints = true; }
			break;
		case 'v':
			show_version = true;
			break;
		case 'h':
			show_help = true;
			break;
		case 'o':
			outputPath = sdsnew(optarg);
			break;
		}
	}
	if (show_help) {
		fprintf(stdout, "otfccbuild version %s\n", VERSION);

		fprintf(stdout, "\n"
		                "Usage : otfccbuild [OPTIONS] input.json -o output.[ttf|otf]\n\n"
		                " -h, --help           : Display this help message and exit.\n"
		                " -v, --version        : Display version information and exit.\n"
		                " -o <file>            : Set output file path to <file>.\n"
		                " --time               : Time each substep.\n"
		                " --ignore-glyph-order : Ignore the glyph order information in the input, except for gid0.\n"
		                " --ignore-hints       : Ignore the hinting information in the input.\n");
		return 0;
	}
	if (show_version) {
		fprintf(stdout, "otfccbuild version %s\n", VERSION);
		return 0;
	}

	if (optind >= argc) {
		fprintf(stderr, "Expected argument after options for input file name. Exit.\n");
		exit(EXIT_FAILURE);
	} else {
		inPath = sdsnew(argv[optind]);
	}
	if (!outputPath) {
		fprintf(stderr, "Unable to build OpenType font tile : output path not specified. Exit.\n");
		exit(EXIT_FAILURE);
	}

	char *buffer = NULL;
	long length = 0;
	{
		FILE *f = fopen(inPath, "rb");
		if (!f) {
			fprintf(stderr, "Cannot read JSON file \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer) { fread(buffer, 1, length, f); }
		fclose(f);
		if (show_time) push_stopwatch("Read file", &begin);
		if (!buffer) {
			fprintf(stderr, "Cannot read JSON file \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
	}
	json_value *root;
	{
		root = json_parse(buffer, length);
		free(buffer);
		if (show_time) push_stopwatch("Parse JSON", &begin);
		if (!root) {
			fprintf(stderr, "Cannot parse JSON file \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
	}

	caryll_font *font;
	{
		font = caryll_font_new();
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
		}
		json_value_free(root);
		if (show_time) push_stopwatch("Convert JSON to font", &begin);
	}
	{
		caryll_font_consolidate(font);
		caryll_font_stat(font);
		if (show_time) push_stopwatch("Consolidation and Stating", &begin);
	}
	{
		caryll_buffer *bufglyf = bufnew();
		caryll_buffer *bufloca = bufnew();
		caryll_write_glyf(font->glyf, font->head, bufglyf, bufloca);

		sfnt_builder *builder = sfnt_builder_new();
		sfnt_builder_push_table(builder, 'head', caryll_write_head(font->head));
		sfnt_builder_push_table(builder, 'hhea', caryll_write_hhea(font->hhea));
		sfnt_builder_push_table(builder, 'OS/2', caryll_write_OS_2(font->OS_2));
		sfnt_builder_push_table(builder, 'maxp', caryll_write_maxp(font->maxp));
		sfnt_builder_push_table(builder, 'name', caryll_write_name(font->name));
		sfnt_builder_push_table(builder, 'post', caryll_write_post(font->post));
		sfnt_builder_push_table(builder, 'cmap', caryll_write_cmap(font->cmap));
		if (font->fpgm) sfnt_builder_push_table(builder, 'fpgm', caryll_write_fpgm_prep(font->fpgm));
		if (font->prep) sfnt_builder_push_table(builder, 'prep', caryll_write_fpgm_prep(font->prep));
		if (font->cvt_) sfnt_builder_push_table(builder, 'cvt ', caryll_write_fpgm_prep(font->cvt_));
		sfnt_builder_push_table(builder, 'hmtx',
		                        caryll_write_hmtx(font->hmtx, font->hhea->numberOfMetrics,
		                                          font->maxp->numGlyphs - font->hhea->numberOfMetrics));
		sfnt_builder_push_table(builder, 'loca', bufloca);
		sfnt_builder_push_table(builder, 'glyf', bufglyf);

		caryll_buffer *otf = sfnt_builder_serialize(builder);
		FILE *outfile = fopen(outputPath, "wb");
		fwrite(otf->s, sizeof(uint8_t), buflen(otf), outfile);
		fclose(outfile);
		if (show_time) push_stopwatch("Write OpenType", &begin);

		sfnt_builder_delete(builder);
		buffree(otf);
		caryll_font_close(font);
		if (show_time) push_stopwatch("Finalize", &begin);
	}
	return 0;
}
