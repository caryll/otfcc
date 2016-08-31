#ifndef CARYLL_SUPPORT_HANDLE_H
#define CARYLL_SUPPORT_HANDLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <extern/sds.h>

typedef enum { HANDLE_STATE_EMPTY, HANDLE_STATE_INDEX, HANDLE_STATE_NAME, HANDLE_STATE_CONSOLIDATED } handle_state;
struct _caryll_handle {
	handle_state state;
	uint16_t index;
	sds name;
};

struct _caryll_handle handle_new();
struct _caryll_handle handle_fromIndex(uint16_t id);
struct _caryll_handle handle_fromName(sds s);
void handle_delete(struct _caryll_handle *h);
void handle_forceDelete(struct _caryll_handle *h);
void handle_consolidateTo(struct _caryll_handle *h, uint16_t id, sds name);

#endif
