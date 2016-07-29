#ifndef CARYLL_SUPPORT_OPTIONS_H
#define CARYLL_SUPPORT_OPTIONS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	bool ignore_glyph_order;
	bool ignore_hints;
	bool has_vertical_metrics;
	bool export_fdselect;
	bool keep_average_char_width;
	bool short_post;
	bool dummy_DSIG;
	bool keep_modified_time;
	bool instr_as_bytes;
	bool verbose;
	bool cff_short_vmtx;
	char *glyph_name_prefix;
	uint8_t optimize_level;
} caryll_options;

caryll_options *caryll_new_options();
void caryll_delete_options(caryll_options *options);

#endif
