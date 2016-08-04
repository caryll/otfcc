#ifndef CARYLL_TABLES_OTL_GEDF_H
#define CARYLL_TABLES_OTL_GEDF_H

#include "otl.h"

typedef struct {
	int8_t format;
	int16_t coordiante;
	int16_t pointIndex;
} caret_value;
typedef struct {
	uint16_t caretCount;
	caret_value *values;
} caret_value_record;
typedef struct {
	otl_coverage *coverage;
	caret_value_record *carets;
} lig_caret_table;

typedef struct {
	otl_classdef *glyphClassDef;
	otl_classdef *markAttachClassDef;
	lig_caret_table *ligCarets;
} table_GDEF;

void caryll_delete_GDEF(table_GDEF *gdef);
table_GDEF *caryll_read_GDEF(caryll_packet packet);
void caryll_GDEF_to_json(table_GDEF *gdef, json_value *root, const caryll_options *options);
table_GDEF *caryll_GDEF_from_json(json_value *root, const caryll_options *options);
caryll_buffer *caryll_write_GDEF(table_GDEF *gdef, const caryll_options *options);

#endif
