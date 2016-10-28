#include "otfcc/sfnt.h"
#include "otfcc/font.h"
#include "otfcc/sfnt-builder.h"

#include "platform.h"
#include "stopwatch.h"

#include <getopt.h>
#include "support/aliases.h"

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
	                "                               --force-cid\n"
	                "                               --subroutinize\n"
	                " --time                    : Time each substep.\n"
	                " --verbose                 : Show more information when building.\n\n"
	                " --ignore-hints            : Ignore the hinting information in the input.\n"
	                " --keep-average-char-width : Keep the OS/2.xAvgCharWidth value from the input\n"
	                "                             instead of stating the average width of glyphs.\n"
	                "                             Useful when creating a monospaced font.\n"
	                " --keep-unicode-ranges     : Keep the OS/2.ulUnicodeRange[1-4] as-is.\n"
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
	                " --force-cid               : Convert name-keyed CFF OTF into CID-keyed.\n"
	                " --subroutinize            : Subroutinize CFF table.\n"
	                " --stub-cmap4              : Create a stub `cmap` format 4 subtable if format\n"
	                "                             12 subtable is present.\n"
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
	static const long BUF_SIZE = 0x400000;
	static const long BUF_MIN = 0x1000;
	char *buffer = malloc(BUF_SIZE);
	long length = 0;
	long remain = BUF_SIZE;
	while (!feof(stdin)) {
		if (remain <= BUF_MIN) {
			remain += (length >> 1) & 0xFFFFFF;
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
	sds outputPath = NULL;
	sds inPath = NULL;
	int option_index = 0;
	int c;

	otfcc_Options *options = options_new();
	options->logger = otfcc_new_Logger(otfcc_new_StdErrTarget());
	options->logger->indent(options->logger, "otfccbuild");

	struct option longopts[] = {{"version", no_argument, NULL, 'v'},
	                            {"help", no_argument, NULL, 'h'},
	                            {"time", no_argument, NULL, 0},
	                            {"ignore-glyph-order", no_argument, NULL, 0},
	                            {"keep-glyph-order", no_argument, NULL, 0},
	                            {"dont-ignore-glyph-order", no_argument, NULL, 0},
	                            {"ignore-hints", no_argument, NULL, 0},
	                            {"keep-average-char-width", no_argument, NULL, 0},
	                            {"keep-unicode-ranges", no_argument, NULL, 0},
	                            {"keep-modified-time", no_argument, NULL, 0},
	                            {"merge-lookups", no_argument, NULL, 0},
	                            {"merge-features", no_argument, NULL, 0},
	                            {"dont-merge-lookups", no_argument, NULL, 0},
	                            {"dont-merge-features", no_argument, NULL, 0},
	                            {"short-post", no_argument, NULL, 0},
	                            {"force-cid", no_argument, NULL, 0},
	                            {"subroutinize", no_argument, NULL, 0},
	                            {"stub-cmap4", no_argument, NULL, 0},
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
				} else if (strcmp(longopts[option_index].name, "ignore-hints") == 0) {
					options->ignore_hints = true;
				} else if (strcmp(longopts[option_index].name, "keep-average-char-width") == 0) {
					options->keep_average_char_width = true;
				} else if (strcmp(longopts[option_index].name, "keep-unicode-ranges") == 0) {
					options->keep_unicode_ranges = true;
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
				} else if (strcmp(longopts[option_index].name, "force-cid") == 0) {
					options->force_cid = true;
				} else if (strcmp(longopts[option_index].name, "subroutinize") == 0) {
					options->cff_doSubroutinize = true;
				} else if (strcmp(longopts[option_index].name, "stub-cmap4") == 0) {
					options->stub_cmap4 = true;
				} else if (strcmp(longopts[option_index].name, "ship") == 0) {
					options->ignore_glyph_order = true;
					options->short_post = true;
					options->dummy_DSIG = true;
				} else if (strcmp(longopts[option_index].name, "verbose") == 0) {
					options->verbose = true;
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
				options_optimizeTo(options, atoi(optarg));
				break;
		}
	}
	options->logger->setVerbosity(options->logger, options->verbose ? 0xFF : 1);
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
		logError("Unable to build OpenType font tile : output path not "
		         "specified. Exit.\n");
		printHelp();
		exit(EXIT_FAILURE);
	}

	char *buffer;
	long length;
	loggedStep("Load file") {
		if (inPath) {
			loggedStep("Load from file %s", inPath) {
				readEntireFile(inPath, &buffer, &length);
				sdsfree(inPath);
			}
		} else {
			loggedStep("Load from stdin") {
				readEntireStdin(&buffer, &length);
			}
		}
		logStepTime;
	}

	json_value *root;
	loggedStep("Parse into JSON") {
		root = json_parse(buffer, length);
		free(buffer);
		logStepTime;
		if (!root) {
			logError("Cannot parse JSON file \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
	}

	caryll_Font *font;
	loggedStep("Parse") {
		font = caryll_parse_Font(root, options);
		if (!font) {
			logError("Cannot parse JSON file \"%s\" as a font. Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		json_value_free(root);
		logStepTime;
	}
	loggedStep("Consolidate") {
		caryll_font_consolidate(font, options);
		caryll_font_stat(font, options);
		logStepTime;
	}
	loggedStep("Build") {
		caryll_Buffer *otf = caryll_build_Font(font, options);
		FILE *outfile = u8fopen(outputPath, "wb");
		fwrite(otf->data, sizeof(uint8_t), buflen(otf), outfile);
		fclose(outfile);
		logStepTime;
		sdsfree(outputPath);
		buffree(otf);
		caryll_delete_Font(font);
	}
	options_delete(options);
	return 0;
}
