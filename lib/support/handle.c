#include "otfcc/handle.h"

struct otfcc_Handle handle_new() {
	struct otfcc_Handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	return h;
}
struct otfcc_Handle handle_copy(struct otfcc_Handle h) {
	struct otfcc_Handle h1 = h;
	if (h.state == HANDLE_STATE_NAME) { h1.name = sdsdup(h.name); }
	return h1;
}
struct otfcc_Handle handle_fromIndex(glyphid_t id) {
	struct otfcc_Handle h = {HANDLE_STATE_INDEX, id, NULL};
	return h;
}
struct otfcc_Handle handle_fromName(sds s) {
	struct otfcc_Handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	if (s) {
		h.state = HANDLE_STATE_NAME;
		h.name = s;
	}
	return h;
}
struct otfcc_Handle handle_fromConsolidated(glyphid_t id, sds s) {
	struct otfcc_Handle h = {HANDLE_STATE_CONSOLIDATED, id, s};
	return h;
}
void handle_delete(struct otfcc_Handle *h) {
	if (h->state == HANDLE_STATE_NAME) { sdsfree(h->name); };
	h->name = NULL;
	h->index = 0;
	h->state = HANDLE_STATE_EMPTY;
}
void handle_forceDelete(struct otfcc_Handle *h) {
	sdsfree(h->name);
	h->name = NULL;
	h->index = 0;
	h->state = HANDLE_STATE_EMPTY;
}
void handle_consolidateTo(struct otfcc_Handle *h, glyphid_t id, sds name) {
	handle_delete(h);
	h->state = HANDLE_STATE_CONSOLIDATED;
	h->index = id;
	h->name = name;
}
