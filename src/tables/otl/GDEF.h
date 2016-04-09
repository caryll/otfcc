#ifndef CARYLL_TABLES_OTL_GEDF_H
#define CARYLL_TABLES_OTL_GEDF_H

#include "otl.h"

typedef struct {
	otl_classdef *glyphClassDef;
} table_GDEF;

void caryll_delete_GDEF(table_GDEF *gdef);
table_GDEF *caryll_read_GDEF(caryll_packet packet);
void caryll_GDEF_to_json(table_GDEF *gdef, json_value *root, caryll_dump_options *dumpopts);
table_GDEF *caryll_GDEF_from_json(json_value *root, caryll_dump_options *dumpopts);
caryll_buffer *caryll_write_GDEF(table_GDEF *gdef);

#endif
