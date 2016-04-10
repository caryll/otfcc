#ifndef CARYLL_FONTOPS_OTL_GSUB_LIGATURE_H
#define CARYLL_FONTOPS_OTL_GSUB_LIGATURE_H
#include "../../caryll-font.h"
#include "common.h"

bool consolidate_gsub_ligature(caryll_font *font, table_otl *table, otl_subtable *_subtable,
                               sds lookupName);

#endif
