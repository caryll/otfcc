#ifndef CARYLL_FONTOPS_OTL_COMMON_H
#define CARYLL_FONTOPS_OTL_COMMON_H

#include "otfcc/font.h"
#include "tables/otl/otl-private.h"

void fontop_consolidateCoverage(caryll_Font *font, otl_Coverage *coverage, const otfcc_Options *options);
void fontop_consolidateClassDef(caryll_Font *font, otl_ClassDef *cd, const otfcc_Options *options);
void fontop_shrinkClassDef(otl_ClassDef *cd);

#endif
