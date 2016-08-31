#include "handle.h"

struct _caryll_handle handle_new() {
	struct _caryll_handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	return h;
}
struct _caryll_handle handle_fromIndex(uint16_t id) {
	struct _caryll_handle h = {HANDLE_STATE_INDEX, id, NULL};
	return h;
}
struct _caryll_handle handle_fromName(sds s) {
	struct _caryll_handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	if (s) {
		h.state = HANDLE_STATE_NAME;
		h.name = s;
	}
	return h;
}
void handle_delete(struct _caryll_handle *h) {
	if (h->state == HANDLE_STATE_NAME) { sdsfree(h->name); };
	h->name = NULL;
	h->index = 0;
	h->state = HANDLE_STATE_EMPTY;
}
void handle_forceDelete(struct _caryll_handle *h) {
	sdsfree(h->name);
	h->name = NULL;
	h->index = 0;
	h->state = HANDLE_STATE_EMPTY;
}

void handle_consolidateTo(struct _caryll_handle *h, uint16_t id, sds name) {
	handle_delete(h);
	h->state = HANDLE_STATE_CONSOLIDATED;
	h->index = id;
	h->name = name;
}
