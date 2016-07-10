#ifndef CARYLL_FONTOPS_OTL_GSUB_MULTI_H
#define CARYLL_FONTOPS_OTL_GSUB_MULTI_H
#include "common.h"

bool consolidate_gsub_multi(caryll_font *font, table_otl *table, otl_subtable *_subtable,
                            sds lookupName);

#endif
