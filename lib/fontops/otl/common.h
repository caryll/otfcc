#ifndef CARYLL_FONTOPS_OTL_COMMON_H
#define CARYLL_FONTOPS_OTL_COMMON_H
#include <font/caryll-font.h>

void consolidate_coverage(caryll_font *font, otl_coverage *coverage, sds lookupName);
void shrink_coverage(otl_coverage *coverage, bool dosort);
void consolidate_classdef(caryll_font *font, otl_classdef *cd, sds lookupName);
void shrink_classdef(otl_classdef *cd);

#endif
