#ifndef CARYLL_FONTOPS_OTL_MARK_H
#define CARYLL_FONTOPS_OTL_MARK_H
#include "common.h"

bool consolidate_mark_to_single(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName);
bool consolidate_mark_to_ligature(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName);

#endif
