#ifndef CARYLL_INCLUDE_OTFCC_HANDLE_H
#define CARYLL_INCLUDE_OTFCC_HANDLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "dep/sds.h"
#include "caryll/ownership.h"
#include "caryll/element.h"
#include "primitives.h"

typedef enum { HANDLE_STATE_EMPTY, HANDLE_STATE_INDEX, HANDLE_STATE_NAME, HANDLE_STATE_CONSOLIDATED } handle_state;
struct otfcc_Handle {
	handle_state state;
	glyphid_t index;
	OWNING sds name;
};

typedef struct otfcc_Handle otfcc_Handle;
typedef struct otfcc_Handle otfcc_GlyphHandle;
typedef struct otfcc_Handle otfcc_FDHandle;
typedef struct otfcc_Handle otfcc_LookupHandle;

struct otfcc_HandlePackage {
	caryll_VT(otfcc_Handle);
	struct otfcc_Handle (*fromIndex)(glyphid_t id);
	struct otfcc_Handle (*fromName)(MOVE sds s);
	struct otfcc_Handle (*fromConsolidated)(glyphid_t id, COPY sds s);
	void (*consolidateTo)(struct otfcc_Handle *h, glyphid_t id, sds name);
};

extern const struct otfcc_HandlePackage otfcc_pkgHandle;

#endif
