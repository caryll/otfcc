#ifndef CARYLL_FONTOPS_CONSOLIDATE_H
#define CARYLL_FONTOPS_CONSOLIDATE_H
#include "font/caryll-font.h"

#include "otl/gsub-single.h"
#include "otl/gsub-multi.h"
#include "otl/gsub-ligature.h"
#include "otl/gsub-reverse.h"
#include "otl/gpos-single.h"
#include "otl/gpos-pair.h"
#include "otl/gpos-cursive.h"
#include "otl/chaining.h"
#include "otl/mark.h"
#include "otl/GDEF.h"

void caryll_font_consolidate(caryll_Font *font, const caryll_Options *options);

#endif
