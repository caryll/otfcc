#ifndef CARYLL_INCLUDE_TABLE_GEDF_H
#define CARYLL_INCLUDE_TABLE_GEDF_H

#include "otl.h"

typedef struct {
	int8_t format;
	pos_t coordiante;
	int16_t pointIndex;
} otl_CaretValue;
typedef struct {
	glyphid_t caretCount;
	OWNING otl_CaretValue *values;
} otl_CaretValueRecord;
typedef struct {
	OWNING otl_Coverage *coverage;
	OWNING otl_CaretValueRecord *carets;
} otl_LigCaretTable;

typedef struct {
	OWNING otl_ClassDef *glyphClassDef;
	OWNING otl_ClassDef *markAttachClassDef;
	OWNING otl_LigCaretTable *ligCarets;
} table_GDEF;

#endif
