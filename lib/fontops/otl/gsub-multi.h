#ifndef CARYLL_FONTOPS_OTL_GSUB_MULTI_H
#define CARYLL_FONTOPS_OTL_GSUB_MULTI_H
#include "common.h"

bool consolidate_gsub_multi(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName);

#endif
