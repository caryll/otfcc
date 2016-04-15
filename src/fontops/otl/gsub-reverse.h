#ifndef CARYLL_FONTOPS_OTL_GSUB_REVERSE_H
#define CARYLL_FONTOPS_OTL_GSUB_REVERSE_H
#include "common.h"

bool consolidate_gsub_reverse(caryll_font *font, table_otl *table, otl_subtable *_subtable,
                              sds lookupName);

#endif
