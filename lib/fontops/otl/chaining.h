#ifndef CARYLL_FONTOPS_OTL_CHAINING_H
#define CARYLL_FONTOPS_OTL_CHAINING_H
#include "common.h"

bool consolidate_chaining(caryll_font *font, table_otl *table, otl_subtable *_subtable,
                          sds lookupName);
void classify(otl_lookup *lookup);

#endif
