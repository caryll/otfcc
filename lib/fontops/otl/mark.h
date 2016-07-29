#ifndef CARYLL_FONTOPS_OTL_MARK_H
#define CARYLL_FONTOPS_OTL_MARK_H
#include "common.h"

bool consolidate_mark_to_single(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName);
bool consolidate_mark_to_ligature(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName);

#endif
