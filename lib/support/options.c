#include "otfcc/options.h"

otfcc_Options *options_new() {
	otfcc_Options *options = calloc(1, sizeof(otfcc_Options));
	options->cff_rollCharString = true;
	return options;
}
void options_delete(otfcc_Options *options) {
	if (options) {
		free(options->glyph_name_prefix);
		if (options->logger) options->logger->dispose(options->logger);
	}
	free(options);
}
void options_optimizeTo(otfcc_Options *options, uint8_t level) {
	if (level >= 1) { options->cff_rollCharString = true; }
	if (level >= 2) {
		options->short_post = true;
		options->ignore_glyph_order = true;
		options->cff_short_vmtx = true;
		options->merge_features = true;
	}
	if (level >= 3) {
		options->force_cid = true;
		options->cff_doSubroutinize = true;
	}
}
