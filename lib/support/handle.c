#include "handle.h"

struct _caryll_handle handle_new() {
	struct _caryll_handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	return h;
}
struct _caryll_handle handle_from_id(uint16_t id) {
	struct _caryll_handle h = {HANDLE_STATE_INDEX, id, NULL};
	return h;
}
struct _caryll_handle handle_from_name(sds s) {
	struct _caryll_handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	if (s) {
		h.state = HANDLE_STATE_NAME;
		h.name = s;
	}
	return h;
}
