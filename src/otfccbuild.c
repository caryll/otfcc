#include <font/caryll-font.h>
#include <font/caryll-sfnt-builder.h>
#include <font/caryll-sfnt.h>
#include <fontops/fontop.h>

#include "platform.h"
#include "stopwatch.h"

#include <getopt.h>

#ifndef MAIN_VER
#define MAIN_VER 0
#endif
#ifndef SECONDARY_VER
#define SECONDARY_VER 0
#endif
#ifndef PATCH_VER
#define PATCH_VER 0
#endif

void printInfo() {
	fprintf(stdout, "This is otfccbuild, version %d.%d.%d.\n", MAIN_VER, SECONDARY_VER, PATCH_VER);
}
void printHelp() {
	fprintf(stdout, "\n"
	                "Usage : otfccbuild [OPTIONS] [input.json] -o output.[ttf|otf]\n\n"
	                " input.json                : Path to input file. When absent the input will be\n"
	                "                             read from the STDIN.\n\n"
	                " -h, --help                : Display this help message and exit.\n"
	                " -v, --version             : Display version information and exit.\n"
	                " -o <file>                 : Set output file path to <file>.\n"
	                " -s, --dummy-dsig          : Include an empty DSIG table in the font. For some\n"
	                "                             Microsoft applications, DSIG is required to enable\n"
	                "                             OpenType features.\n"
	                " -O<n>                     : Specify the level for optimization.\n"
	                "     -O0                     Turn off any optimization.\n"
	                "     -O1                     Default optimization.\n"
	                "     -O2                     More aggressive optimizations for web font. In this\n"
	                "                             level, the following options will be set:\n"
	                "                               --ignore-glyph-order\n"
	                "                               --short-post\n"
	                "                               --merge-features\n"
	                "     -O3                     Most aggressive opptimization strategy will be\n"
	                "                             used. In this level, these options will be set:\n"
	                "                               --merge-lookups\n"
	                " --time                    : Time each substep.\n"
	                " --verbose                 : Show more information when building.\n\n"
	                " --ignore-hints            : Ignore the hinting information in the input.\n"
	                " --keep-average-char-width : Keep the OS/2.xAvgCharWidth value from the input\n"
	                "                             instead of stating the average width of glyphs.\n"
	                "                             Useful when creating a monospaced font.\n"
	                " --keep-modified-time      : Keep the head.modified time in the json, instead of\n"
	                "                             using current time.\n\n"
	                " --short-post              : Don't export glyph names in the result font.\n"
	                " --ignore-glyph-order      : Ignore the glyph order information in the input.\n"
	                " --keep-glyph-order        : Keep the glyph order information in the input.\n"
	                "                             Use to preserve glyph order under -O2 and -O3.\n"
	                " --dont-ignore-glyph-order : Same as --keep-glyph-order.\n"
	                " --merge-features          : Merge duplicate OpenType feature definitions.\n"
	                " --dont-merge-features     : Keep duplicate OpenType feature definitions.\n"
	                " --merge-lookups           : Merge duplicate OpenType lookups.\n"
	                " --dont-merge-lookups      : Keep duplicate OpenType lookups.\n"
	                "\n");
}
void readEntireFile(char *inPath, char **_buffer, long *_length) {
	char *buffer = NULL;
	long length = 0;
	FILE *f = u8fopen(inPath, "rb");
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

	if (!buffer) {
		fprintf(stderr, "Cannot read JSON file \"%s\". Exit.\n", inPath);
		exit(EXIT_FAILURE);
	}
	*_buffer = buffer;
	*_length = length;
}

void readEntireStdin(char **_buffer, long *_length) {
	freopen(NULL, "rb", stdin);
	static const long BUF_SIZE = 0x400;
	static const long BUF_MIN = 0x100;
	char *buffer = malloc(BUF_SIZE);
	long length = 0;
	long remain = BUF_SIZE;
	while (!feof(stdin)) {
		if (remain <= BUF_MIN) {
			remain += (length >> 1) & 0xFFFF;
			buffer = realloc(buffer, length + remain);
		}

		fgets(buffer + length, remain, stdin);
		long n = (long)strlen(buffer + length);
		length += n;
		remain -= n;
	}
	*_buffer = buffer;
	*_length = length;
}

void print_table(sfnt_builder_entry *t) {
	fprintf(stderr, "Writing Table %c%c%c%c, Length: %8d, Checksum: %08X\n", ((uint32_t)(t->tag) >> 24) & 0xff,
	        ((uint32_t)(t->tag) >> 16) & 0xff, ((uint32_t)(t->tag) >> 8) & 0xff, t->tag & 0xff, t->length, t->checksum);
}

#ifdef _WIN32
int main() {
	int argc;
	char **argv;
	get_argv_utf8(&argc, &argv);
#else
int main(int argc, char *argv[]) {
#endif
	struct timespec begin;
	time_now(&begin);

	bool show_help = false;
	bool show_version = false;
	bool show_time = false;
	sds outputPath = NULL;
	sds inPath = NULL;
	int option_index = 0;
	int c;

	caryll_options *options = caryll_new_options();

	struct option longopts[] = {{"version", no_argument, NULL, 'v'},
	                            {"help", no_argument, NULL, 'h'},
	                            {"time", no_argument, NULL, 0},
	                            {"ignore-glyph-order", no_argument, NULL, 0},
	                            {"keep-glyph-order", no_argument, NULL, 0},
	                            {"dont-ignore-glyph-order", no_argument, NULL, 0},
	                            {"ignore-hints", no_argument, NULL, 0},
	                            {"keep-average-char-width", no_argument, NULL, 0},
	                            {"keep-modified-time", no_argument, NULL, 0},
	                            {"merge-lookups", no_argument, NULL, 0},
	                            {"merge-features", no_argument, NULL, 0},
	                            {"dont-merge-lookups", no_argument, NULL, 0},
	                            {"dont-merge-features", no_argument, NULL, 0},
	                            {"short-post", no_argument, NULL, 0},
	                            {"dummy-dsig", no_argument, NULL, 's'},
	                            {"ship", no_argument, NULL, 0},
	                            {"verbose", no_argument, NULL, 0},
	                            {"optimize", required_argument, NULL, 'O'},
	                            {"output", required_argument, NULL, 'o'},
	                            {0, 0, 0, 0}};

	while ((c = getopt_long(argc, argv, "vhsO:o:", longopts, &option_index)) != (-1)) {
		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (longopts[option_index].flag != 0) {
					break;
				} else if (strcmp(longopts[option_index].name, "time") == 0) {
					show_time = true;
				} else if (strcmp(longopts[option_index].name, "ignore-hints") == 0) {
					options->ignore_hints = true;
				} else if (strcmp(longopts[option_index].name, "keep-average-char-width") == 0) {
					options->keep_average_char_width = true;
				} else if (strcmp(longopts[option_index].name, "keep-modified-time") == 0) {
					options->keep_modified_time = true;
				} else if (strcmp(longopts[option_index].name, "merge-features") == 0) {
					options->merge_features = true;
				} else if (strcmp(longopts[option_index].name, "merge-lookups") == 0) {
					options->merge_lookups = true;
				} else if (strcmp(longopts[option_index].name, "dont-merge-features") == 0) {
					options->merge_features = false;
				} else if (strcmp(longopts[option_index].name, "dont-merge-lookups") == 0) {
					options->merge_lookups = false;
				} else if (strcmp(longopts[option_index].name, "ignore-glyph-order") == 0) {
					options->ignore_glyph_order = true;
				} else if (strcmp(longopts[option_index].name, "keep-glyph-order") == 0) {
					options->ignore_glyph_order = false;
				} else if (strcmp(longopts[option_index].name, "dont-keep-glyph-order") == 0) {
					options->ignore_glyph_order = false;
				} else if (strcmp(longopts[option_index].name, "short-post") == 0) {
					options->short_post = true;
				} else if (strcmp(longopts[option_index].name, "ship") == 0) {
					options->ignore_glyph_order = true;
					options->short_post = true;
					options->dummy_DSIG = true;
				} else if (strcmp(longopts[option_index].name, "verbose") == 0) {
					options->verbose = true;
					show_time = true;
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
			case 's':
				options->dummy_DSIG = true;
				break;
			case 'O':
				options->optimize_level = atoi(optarg);
				if (options->optimize_level >= 2) {
					options->short_post = true;
					options->ignore_glyph_order = true;
					options->cff_short_vmtx = true;
					options->merge_features = true;
				}
				if (options->optimize_level >= 3) { options->merge_lookups = true; }
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
		inPath = NULL; // read from STDIN
	} else {
		inPath = sdsnew(argv[optind]); // read from file
	}
	if (!outputPath) {
		fprintf(stderr, "Unable to build OpenType font tile : output path not "
		                "specified. Exit.\n");
		printHelp();
		exit(EXIT_FAILURE);
	}

	char *buffer;
	long length;
	{
		if (inPath) {
			if (options->verbose) { fprintf(stderr, "Building OpenType font from %s to %s.\n", inPath, outputPath); }
			readEntireFile(inPath, &buffer, &length);
		} else {
			if (options->verbose) { fprintf(stderr, "Building OpenType font from %s to %s.\n", "[STDIN]", outputPath); }
			readEntireStdin(&buffer, &length);
		}
		if (show_time) push_stopwatch("Read input", &begin);
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
		font = caryll_font_from_json(root, options);
		if (!font) {
			fprintf(stderr, "Cannot parse JSON file \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		json_value_free(root);
		if (show_time) push_stopwatch("Convert JSON to font", &begin);
	}
	{
		caryll_font_consolidate(font, options);
		if (show_time) push_stopwatch("Consolidation", &begin);
		caryll_font_stat(font, options);
		if (show_time) push_stopwatch("Stating", &begin);
	}
	{
		caryll_buffer *otf = caryll_write_font(font, options);
		FILE *outfile = u8fopen(outputPath, "wb");
		fwrite(otf->data, sizeof(uint8_t), buflen(otf), outfile);
		fclose(outfile);
		if (show_time) push_stopwatch("Write OpenType", &begin);

		buffree(otf);
		caryll_delete_font(font);
		caryll_delete_options(options);
		if (show_time) push_stopwatch("Finalize", &begin);
	}
	return 0;
}
