#ifndef CARYLL_FONTOPS_OTL_CHAINING_H
#define CARYLL_FONTOPS_OTL_CHAINING_H
#include "common.h"

bool consolidate_chaining(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName);
void fontop_classifyChainings(otl_Lookup *lookup);

#endif
