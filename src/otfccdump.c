#include "font/caryll-font.h"
#include "font/caryll-sfnt.h"
#include "fontops/fontop.h"

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
	fprintf(stdout, "This is otfccdump, version %d.%d.%d.\n", MAIN_VER, SECONDARY_VER, PATCH_VER);
}
void printHelp() {
	fprintf(stdout, "\n"
	                "Usage : otfccdump [OPTIONS] input.[otf|ttf|ttc]\n\n"
	                " -h, --help              : Display this help message and exit.\n"
	                " -v, --version           : Display version information and exit.\n"
	                " -o <file>               : Set output file path to <file>. When absent the dump\n"
	                "                           will be written to STDOUT.\n"
	                " -n <n>, --ttc-index <n> : Use the <n>th subfont within the input font.\n"
	                " --pretty                : Prettify the output JSON.\n"
	                " --ugly                  : Force uglify the output JSON.\n"
	                " --time                  : Time each substep.\n"
	                " --verbose               : Show more information when building.\n"
	                " --ignore-glyph-order    : Do not export glyph order information.\n"
	                " --glyph-name-prefix pfx : Add a prefix to the glyph names.\n"
	                " --ignore-hints          : Do not export hinting information.\n"
	                " --add-bom               : Add BOM mark in the output. (It is default on Windows\n"
	                "                           when redirecting to another program. Use --no-bom to\n"
	                "                           turn it off.)\n"
	                "\n");
}
#ifdef _WIN32
int main() {
	int argc;
	char **argv;
	get_argv_utf8(&argc, &argv);
#else
int main(int argc, char *argv[]) {
#endif

	bool show_help = false;
	bool show_version = false;
	bool show_pretty = false;
	bool show_ugly = false;
	bool show_time = false;
	bool add_bom = false;
	bool no_bom = false;
	uint32_t ttcindex = 0;
	struct option longopts[] = {{"version", no_argument, NULL, 'v'},
	                            {"help", no_argument, NULL, 'h'},
	                            {"pretty", no_argument, NULL, 'p'},
	                            {"ugly", no_argument, NULL, 0},
	                            {"time", no_argument, NULL, 0},
	                            {"ignore-glyph-order", no_argument, NULL, 0},
	                            {"ignore-hints", no_argument, NULL, 0},
	                            {"instr-as-bytes", no_argument, NULL, 0},
	                            {"glyph-name-prefix", required_argument, NULL, 0},
	                            {"verbose", no_argument, NULL, 0},
	                            {"add-bom", no_argument, NULL, 0},
	                            {"no-bom", no_argument, NULL, 0},
	                            {"output", required_argument, NULL, 'o'},
	                            {"ttc-index", required_argument, NULL, 'n'},
	                            {0, 0, 0, 0}};

	caryll_Options *options = options_new();

	int option_index = 0;
	int c;

	sds outputPath = NULL;
	sds inPath = NULL;

	while ((c = getopt_long(argc, argv, "vhpo:n:", longopts, &option_index)) != (-1)) {
		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (longopts[option_index].flag != 0) {
					break;
				} else if (strcmp(longopts[option_index].name, "ugly") == 0) {
					show_ugly = true;
				} else if (strcmp(longopts[option_index].name, "time") == 0) {
					show_time = true;
				} else if (strcmp(longopts[option_index].name, "add-bom") == 0) {
					add_bom = true;
				} else if (strcmp(longopts[option_index].name, "no-bom") == 0) {
					no_bom = true;
				} else if (strcmp(longopts[option_index].name, "ignore-glyph-order") == 0) {
					options->ignore_glyph_order = true;
				} else if (strcmp(longopts[option_index].name, "verbose") == 0) {
					options->verbose = true;
					show_time = true;
				} else if (strcmp(longopts[option_index].name, "ignore-hints") == 0) {
					options->ignore_hints = true;
				} else if (strcmp(longopts[option_index].name, "instr-as-bytes") == 0) {
					options->instr_as_bytes = true;
				} else if (strcmp(longopts[option_index].name, "glyph-name-prefix") == 0) {
					options->glyph_name_prefix = strdup(optarg);
				}
				break;
			case 'v':
				show_version = true;
				break;
			case 'h':
				show_help = true;
				break;
			case 'p':
				show_pretty = true;
				break;
			case 'o':
				outputPath = sdsnew(optarg);
				break;
			case 'n':
				ttcindex = atoi(optarg);
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
		fprintf(stderr, "Expected argument for input file name.\n");
		printHelp();
		exit(EXIT_FAILURE);
	} else {
		inPath = sdsnew(argv[optind]);
	}

	struct timespec begin;

	time_now(&begin);

	caryll_SplineFontContainer *sfnt;
	{
		FILE *file = u8fopen(inPath, "rb");
		sfnt = caryll_read_SFNT(file);
		if (!sfnt || sfnt->count == 0) {
			fprintf(stderr, "Cannot read SFNT file \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		if (ttcindex >= sfnt->count) {
			fprintf(stderr, "Subfont index %d out of range for \"%s\" (0 -- %d). Exit.\n", ttcindex, inPath,
			        (sfnt->count - 1));
			exit(EXIT_FAILURE);
		}
		if (show_time) push_stopwatch("Read Input SFNT", &begin);
	}

	caryll_Font *font;
	{
		font = caryll_read_Font(sfnt, ttcindex, options);
		if (!font) {
			fprintf(stderr, "Font structure broken or corrupted \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		caryll_font_unconsolidate(font, options);
		if (show_time) push_stopwatch("Parse SFNT", &begin);
	}

	json_value *root;
	{
		root = caryll_dump_Font(font, options);
		if (!root) {
			fprintf(stderr, "Font structure broken or corrupted \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		if (show_time) push_stopwatch("Convert to JSON", &begin);
	}

	char *buf;
	{
		json_serialize_opts options;
		options.mode = json_serialize_mode_packed;
		options.opts = 0;
		options.indent_size = 4;
		if (show_pretty || (!outputPath && isatty(fileno(stdout)))) { options.mode = json_serialize_mode_multiline; }
		if (show_ugly) options.mode = json_serialize_mode_packed;
		buf = malloc(json_measure_ex(root, options));
		json_serialize_ex(buf, root, options);
		if (show_time) push_stopwatch("Serialize to string", &begin);
	}

	{
		if (outputPath) {
			FILE *outputFile = u8fopen(outputPath, "wb");
			if (!outputFile) {
				fprintf(stderr, "Cannot write to file \"%s\". Exit.", outputPath);
				exit(EXIT_FAILURE);
			}
			if (add_bom) {
				fputc(0xEF, outputFile);
				fputc(0xBB, outputFile);
				fputc(0xBF, outputFile);
			}
			fputs(buf, outputFile);
			fclose(outputFile);
		} else {
#ifdef WIN32
			if (isatty(fileno(stdout))) {
				LPWSTR pwStr;
				DWORD dwNum = widen_utf8(buf, &pwStr);
				DWORD actual = 0;
				DWORD written = 0;
				const DWORD chunk = 0x10000;
				while (written < dwNum) {
					DWORD len = dwNum - written;
					if (len > chunk) len = chunk;
					WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), pwStr + written, len, &actual, NULL);
					written += len;
				}
				free(pwStr);
			} else {
				if (!no_bom) {
					fputc(0xEF, stdout);
					fputc(0xBB, stdout);
					fputc(0xBF, stdout);
				}
				fputs(buf, stdout);
			}
#else
			if (add_bom) {
				fputc(0xEF, stdout);
				fputc(0xBB, stdout);
				fputc(0xBF, stdout);
			}
			fputs(buf, stdout);
#endif
		}
		if (show_time) push_stopwatch("Write to file", &begin);
	}

	{
		free(buf);
		if (font) caryll_delete_Font(font);
		if (root) json_builder_free(root);
		if (sfnt) caryll_delete_SFNT(sfnt);
		if (inPath) sdsfree(inPath);
		if (outputPath) sdsfree(outputPath);
		options_delete(options);
		if (show_time) push_stopwatch("Complete", &begin);
	}

	return 0;
}
