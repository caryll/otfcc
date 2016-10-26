#ifndef CARYLL_INCLUDE_OPTIONS_H
#define CARYLL_INCLUDE_OPTIONS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	bool ignore_glyph_order;
	bool ignore_hints;
	bool has_vertical_metrics;
	bool export_fdselect;
	bool keep_average_char_width;
	bool keep_unicode_ranges;
	bool short_post;
	bool dummy_DSIG;
	bool keep_modified_time;
	bool instr_as_bytes;
	bool verbose;
	bool cff_short_vmtx;
	bool merge_lookups;
	bool merge_features;
	bool force_cid;
	bool cff_rollCharString;
	bool cff_doSubroutinize;
	bool stub_cmap4;
	char *glyph_name_prefix;
} otfcc_Options;

otfcc_Options *options_new();
void options_delete(otfcc_Options *options);
void options_optimizeTo(otfcc_Options *options, uint8_t level);

#endif
