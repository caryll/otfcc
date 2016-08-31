#ifndef CARYLL_TABLES_OTL_GEDF_H
#define CARYLL_TABLES_OTL_GEDF_H

#include "otl.h"

typedef struct {
	int8_t format;
	int16_t coordiante;
	int16_t pointIndex;
} otl_CaretValue;
typedef struct {
	uint16_t caretCount;
	otl_CaretValue *values;
} otl_CaretValueRecord;
typedef struct {
	otl_Coverage *coverage;
	otl_CaretValueRecord *carets;
} otl_LigCaretTable;

typedef struct {
	otl_ClassDef *glyphClassDef;
	otl_ClassDef *markAttachClassDef;
	otl_LigCaretTable *ligCarets;
} table_GDEF;

void table_delete_GDEF(table_GDEF *gdef);
table_GDEF *table_read_GDEF(caryll_Packet packet);
void table_dump_GDEF(table_GDEF *gdef, json_value *root, const caryll_Options *options);
table_GDEF *table_parse_GDEF(json_value *root, const caryll_Options *options);
caryll_buffer *table_build_GDEF(table_GDEF *gdef, const caryll_Options *options);

#endif
