#ifndef CARYLL_TABLE_OTL_SUBTABLES_COMMON_H
#define CARYLL_TABLE_OTL_SUBTABLES_COMMON_H

#include "support/util.h"
#include "bk/bkgraph.h"
#include "../../otl.h"

#define checkLength(offset)                                                                                            \
	if (tableLength < offset) { goto FAIL; }

#endif
