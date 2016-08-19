#ifndef CARYLL_BK_BLOCK_H
#define CARYLL_BK_BLOCK_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <support/buffer.h>

struct __caryll_bkblock;
typedef enum {
	bkover = 0,   // nothing
	b8 = 1,       // byte
	b16 = 2,      // short
	b32 = 3,      // long
	p16 = 0x10,   // 16-bit offset, p = pointer to block
	p32 = 0x11,   // 32-bit offset, p = pointer to block
	sp16 = 0x80,  // 16-bit offset, p = pointer to block, marked as compact
	sp32 = 0x81,  // 32-bit offset, p = pointer to block, marked as compact
	bcopy = 0xFE, // Embed another block
	bembed = 0xFF // Embed another block
} bk_cell_type;
typedef enum { VISIT_WHITE, VISIT_GRAY, VISIT_BLACK } bk_cell_visit_state;

typedef struct {
	bk_cell_type t;
	union {
		uint32_t z;
		struct __caryll_bkblock *p;
	};
} bk_cell;

typedef struct __caryll_bkblock {
	bk_cell_visit_state _visitstate;
	uint32_t _index;
	uint32_t _height;
	uint32_t _depth;
	size_t length;
	size_t free;
	bk_cell *cells;
} caryll_bkblock;

caryll_bkblock *_bkblock_init();
caryll_bkblock *new_bkblock(bk_cell_type type0, ...);
caryll_bkblock *bkblock_push(caryll_bkblock *b, bk_cell_type type0, ...);
caryll_bkblock *new_bkblock_from_buffer(/*MOVE*/ caryll_buffer *buf);
bool bk_cell_is_ptr(bk_cell *cell);
void print_bkblock(caryll_bkblock *b);

#endif
