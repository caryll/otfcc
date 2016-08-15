#ifndef CARYLL_BK_BLOCK_H
#define CARYLL_BK_BLOCK_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

struct __caryll_bkblock;
typedef enum {
	bkover = 0, // nothing
	b8 = 1,     // byte
	b16 = 2,    // short
	b32 = 3,    // long
	p16 = 0x10, // 16-bit offset, z = forest index
	p32 = 0x11  // 32-bit offset, z = forest index
} bk_cell_type;

typedef struct {
	bk_cell_type t;
	uint32_t z;
} bk_cell;

typedef struct __caryll_bkblock {
	bool _leaf;
	size_t length;
	size_t free;
	bk_cell *cells;
} caryll_bkblock;

caryll_bkblock *_bkblock_init();
caryll_bkblock *new_bkblock(bk_cell_type type0, ...);

#endif
