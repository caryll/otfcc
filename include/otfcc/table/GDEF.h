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

void otfcc_deleteTableGDEF(MOVE table_GDEF *gdef);
table_GDEF *otfcc_readTableGDEF(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpTableGDEF(const table_GDEF *gdef, json_value *root, const otfcc_Options *options);
table_GDEF *otfcc_parseTableGDEF(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildTableGDEF(const table_GDEF *gdef, const otfcc_Options *options);

#endif
