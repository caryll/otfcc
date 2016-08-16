#ifndef CARYLL_BK_BKGRAPH_H
#define CARYLL_BK_BKGRAPH_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <support/buffer.h>

#include "bkblock.h"

typedef struct {
	uint32_t alias;
	uint32_t order;
	uint32_t height;
	uint32_t hash;
	caryll_bkblock *block;
} bkgraph_entry;

typedef struct {
	size_t length;
	size_t free;
	bkgraph_entry *entries;
} caryll_bkgraph;

caryll_bkgraph *caryll_bkgraph_from_block(caryll_bkblock *b);
void caryll_delete_bkgraph(caryll_bkgraph *f);
void caryll_minimize_bkgraph(caryll_bkgraph *f);
caryll_buffer *caryll_write_bkgraph(caryll_bkgraph *f);

#endif
