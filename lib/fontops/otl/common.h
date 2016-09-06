#ifndef CARYLL_FONTOPS_OTL_COMMON_H
#define CARYLL_FONTOPS_OTL_COMMON_H
#include <font/caryll-font.h>

void fontop_consolidateCoverage(caryll_Font *font, otl_Coverage *coverage, sds lookupName);
void fontop_shrinkCoverage(otl_Coverage *coverage, bool dosort);
void fontop_consolidateClassDef(caryll_Font *font, otl_ClassDef *cd, sds lookupName);
void fontop_shrinkClassDef(otl_ClassDef *cd);

#endif
