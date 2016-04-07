#include "caryll-sfnt.h"
#include "caryll-font.h"
#include "caryll-sfnt-builder.h"

#include <getopt.h>
#include "support/stopwatch.h"
#include "version.h"

void printInfo() {
	fprintf(stdout, "This is otfccbuild, version %s.\n", VERSION);
}
void printHelp() {
	fprintf(stdout, "\n"
	                "Usage : otfccbuild [OPTIONS] input.json -o output.[ttf|otf]\n\n"
	                " -h, --help                : Display this help message and exit.\n"
	                " -v, --version             : Display version information and exit.\n"
	                " -o <file>                 : Set output file path to <file>.\n"
	                " --time                    : Time each substep.\n"
	                " --ignore-glyph-order      : Ignore the glyph order information in the input.\n"
	                " --ignore-hints            : Ignore the hinting information in the input.\n"
	                " --keep-average-char-width : Keep the OS/2.xAvgCharWidth value from the input\n"
	                "                             instead of stating the average width of glyphs.\n"
	                "                             Useful when creating a monospaced font.\n");
}

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
	caryll_dump_options *dumpopts = calloc(1, sizeof(caryll_dump_options));

	struct option longopts[] = {
	    {"version", no_argument, NULL, 'v'},      {"help", no_argument, NULL, 'h'},
	    {"time", no_argument, NULL, 0},           {"ignore-glyph-order", no_argument, NULL, 0},
	    {"ignore-hints", no_argument, NULL, 0},   {"keep-average-char-width", no_argument, NULL, 0},
	    {"output", required_argument, NULL, 'o'}, {0, 0, 0, 0}};

	while ((c = getopt_long(argc, argv, "vhpo:n:", longopts, &option_index)) != (-1)) {
		switch (c) {
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (longopts[option_index].flag != 0) break;
			if (strcmp(longopts[option_index].name, "time") == 0) { show_time = true; }
			if (strcmp(longopts[option_index].name, "ignore-glyph-order") == 0) { dumpopts->ignore_glyph_order = true; }
			if (strcmp(longopts[option_index].name, "ignore-hints") == 0) { dumpopts->ignore_hints = true; }
			if (strcmp(longopts[option_index].name, "keep-average-char-width") == 0) {
				dumpopts->keep_average_char_width = true;
			}
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
		printInfo();
		printHelp();
		return 0;
	}
	if (show_version) {
		printInfo();
		return 0;
	}

	if (optind >= argc) {
		fprintf(stderr, "Expected argument after options for input file name. Exit.\n");
		printHelp();
		exit(EXIT_FAILURE);
	} else {
		inPath = sdsnew(argv[optind]);
	}
	if (!outputPath) {
		fprintf(stderr, "Unable to build OpenType font tile : output path not specified. Exit.\n");
		printHelp();
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
		font = caryll_font_from_json(root, dumpopts);
		if (!font) {
			fprintf(stderr, "Cannot parse JSON file \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		json_value_free(root);
		if (show_time) push_stopwatch("Convert JSON to font", &begin);
	}
	{
		caryll_font_consolidate(font, dumpopts);
		caryll_font_stat(font, dumpopts);
		if (show_time) push_stopwatch("Consolidation and Stating", &begin);
	}
	{
		caryll_buffer *otf = caryll_write_font(font);
		FILE *outfile = fopen(outputPath, "wb");
		fwrite(otf->s, sizeof(uint8_t), buflen(otf), outfile);
		fclose(outfile);
		if (show_time) push_stopwatch("Write OpenType", &begin);

		buffree(otf);
		caryll_delete_font(font);
		if (dumpopts) free(dumpopts);
		if (show_time) push_stopwatch("Finalize", &begin);
	}
	return 0;
}
