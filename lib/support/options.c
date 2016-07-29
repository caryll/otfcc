#include "options.h"

caryll_options *caryll_new_options() {
	caryll_options *options = calloc(1, sizeof(caryll_options));
	options->optimize_level = 1;
	return options;
}
void caryll_delete_options(caryll_options *options) {
	if (options) { free(options->glyph_name_prefix); }
	free(options);
}
