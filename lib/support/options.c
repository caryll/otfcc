#include "options.h"

caryll_Options *options_new() {
	caryll_Options *options = calloc(1, sizeof(caryll_Options));
	options->optimize_level = 1;
	return options;
}
void options_delete(caryll_Options *options) {
	if (options) { free(options->glyph_name_prefix); }
	free(options);
}
