#ifndef CARYLL_INCLUDE_OTFCC_HANDLE_H
#define CARYLL_INCLUDE_OTFCC_HANDLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "dep/sds.h"
#include "caryll/ownership.h"
#include "primitives.h"

typedef enum { HANDLE_STATE_EMPTY, HANDLE_STATE_INDEX, HANDLE_STATE_NAME, HANDLE_STATE_CONSOLIDATED } handle_state;
struct otfcc_Handle {
	handle_state state;
	glyphid_t index;
	OWNING sds name;
};

struct otfcc_Handle handle_new();
struct otfcc_Handle handle_copy(struct otfcc_Handle h);
struct otfcc_Handle handle_fromIndex(glyphid_t id);
struct otfcc_Handle handle_fromName(sds s);
struct otfcc_Handle handle_fromConsolidated(glyphid_t id, sds s);
void handle_delete(struct otfcc_Handle *h);
void handle_consolidateTo(struct otfcc_Handle *h, glyphid_t id, sds name);

typedef struct otfcc_Handle otfcc_GlyphHandle;
typedef struct otfcc_Handle otfcc_FDHandle;
typedef struct otfcc_Handle otfcc_LookupHandle;

#endif
