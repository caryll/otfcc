#ifndef CARYLL_FONTOPS_OTL_GPOS_PAIR_H
#define CARYLL_FONTOPS_OTL_GPOS_PAIR_H
#include "../../caryll-font.h"
#include "common.h"

bool consolidate_gpos_pair(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName);

#endif
