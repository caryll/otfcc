#ifndef CARYLL_TABLE_OTL_PRIVATE_H
#define CARYLL_TABLE_OTL_PRIVATE_H

#include "support/util.h"
#include "bk/bkgraph.h"

#include "otfcc/table/otl.h"

#include "gsub-single.h"
#include "gsub-multi.h"
#include "gsub-ligature.h"
#include "gsub-reverse.h"
#include "gpos-single.h"
#include "gpos-pair.h"
#include "gpos-cursive.h"
#include "gpos-mark-to-single.h"
#include "gpos-mark-to-ligature.h"
#include "chaining.h"
#include "extend.h"

#define checkLength(offset)                                                                                            \
	if (tableLength < offset) { goto FAIL; }

#endif
