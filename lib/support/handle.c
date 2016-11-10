#include "otfcc/handle.h"
#include "support/otfcc-alloc.h"

// default constructors
static void handle_init(otfcc_Handle *h) {
	h->state = HANDLE_STATE_EMPTY;
	h->index = 0;
	h->name = NULL;
}
static struct otfcc_Handle handle_new() {
	struct otfcc_Handle h;
	handle_init(&h);
	return h;
}
static void handle_copy(otfcc_Handle *dst, const otfcc_Handle *src) {
	dst->state = src->state;
	dst->index = src->index;
	if (src->name) {
		dst->name = sdsdup(src->name);
	} else {
		dst->name = NULL;
	}
}
static struct otfcc_Handle handle_dup(struct otfcc_Handle h) {
	struct otfcc_Handle h1;
	handle_copy(&h1, &h);
	return h1;
}
static void handle_dispose(struct otfcc_Handle *h) {
	if (h->name) {
		sdsfree(h->name);
		h->name = NULL;
	}
	h->index = 0;
	h->state = HANDLE_STATE_EMPTY;
}

// custom constructors
static struct otfcc_Handle handle_fromIndex(glyphid_t id) {
	struct otfcc_Handle h = {HANDLE_STATE_INDEX, id, NULL};
	return h;
}
static struct otfcc_Handle handle_fromName(MOVE sds s) {
	struct otfcc_Handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	if (s) {
		h.state = HANDLE_STATE_NAME;
		h.name = s;
	}
	return h;
}
static struct otfcc_Handle handle_fromConsolidated(glyphid_t id, sds s) {
	struct otfcc_Handle h = {HANDLE_STATE_CONSOLIDATED, id, sdsdup(s)};
	return h;
}

// consolidation
static void handle_consolidateTo(struct otfcc_Handle *h, glyphid_t id, sds name) {
	handle_dispose(h);
	h->state = HANDLE_STATE_CONSOLIDATED;
	h->index = id;
	h->name = sdsdup(name);
}

const struct otfcc_HandlePackage otfcc_pkgHandle = {
    .init = handle_init,
    .empty = handle_new,
    .copy = handle_copy,
    .dup = handle_dup,
    .dispose = handle_dispose,
    .fromIndex = handle_fromIndex,
    .fromName = handle_fromName,
    .fromConsolidated = handle_fromConsolidated,
    .consolidateTo = handle_consolidateTo,
};
