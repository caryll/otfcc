#ifndef CARYLL_TABLES_OTL_GEDF_H
#define CARYLL_TABLES_OTL_GEDF_H

#include "otl.h"

typedef struct {
	int8_t format;
	pos_t coordiante;
	int16_t pointIndex;
} otl_CaretValue;
typedef struct {
	glyphid_t caretCount;
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

void table_delete_GDEF(MOVE table_GDEF *gdef);
table_GDEF *table_read_GDEF(const caryll_Packet packet);
void table_dump_GDEF(const table_GDEF *gdef, json_value *root, const caryll_Options *options);
table_GDEF *table_parse_GDEF(const json_value *root, const caryll_Options *options);
caryll_Buffer *table_build_GDEF(const table_GDEF *gdef, const caryll_Options *options);

#endif
