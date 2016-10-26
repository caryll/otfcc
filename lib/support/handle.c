#include "otfcc/handle.h"

struct otfcc_Handle handle_new() {
	struct otfcc_Handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	return h;
}
struct otfcc_Handle handle_copy(struct otfcc_Handle h) {
	struct otfcc_Handle h1 = h;
	h1.name = sdsdup(h.name);
	return h1;
}
struct otfcc_Handle handle_fromIndex(glyphid_t id) {
	struct otfcc_Handle h = {HANDLE_STATE_INDEX, id, NULL};
	return h;
}
struct otfcc_Handle handle_fromName(MOVE sds s) {
	struct otfcc_Handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	if (s) {
		h.state = HANDLE_STATE_NAME;
		h.name = s;
	}
	return h;
}
struct otfcc_Handle handle_fromConsolidated(glyphid_t id, sds s) {
	struct otfcc_Handle h = {HANDLE_STATE_CONSOLIDATED, id, sdsdup(s)};
	return h;
}
void handle_dispose(struct otfcc_Handle *h) {
	if (h->name) {
		sdsfree(h->name);
		h->name = NULL;
	}
	h->index = 0;
	h->state = HANDLE_STATE_EMPTY;
}
void handle_delete(struct otfcc_Handle *h) {
	handle_dispose(h);
	free(h);
}
void handle_consolidateTo(struct otfcc_Handle *h, glyphid_t id, sds name) {
	handle_dispose(h);
	h->state = HANDLE_STATE_CONSOLIDATED;
	h->index = id;
	h->name = sdsdup(name);
}
