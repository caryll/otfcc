#include "bkblock.h"

static void bkblock_acells(caryll_bkblock *b, size_t len) {
	if (len <= b->length + b->free) {
		// We have enough space
		b->free -= len - b->length;
		b->length = len;
	} else {
		// allocate space
		b->length = len;
		b->free = (len >> 1) & 0xFFFFFF;
		if (b->cells) {
			b->cells = realloc(b->cells, sizeof(bk_cell) * (b->length + b->free));
		} else {
			b->cells = malloc(sizeof(bk_cell) * (b->length + b->free));
		}
	}
}
static bk_cell *bkblock_grow(caryll_bkblock *b, size_t len) {
	size_t olen = b->length;
	bkblock_acells(b, olen + len);
	return &(b->cells[olen]);
}

caryll_bkblock *_bkblock_init() {
	caryll_bkblock *b = calloc(1, sizeof(caryll_bkblock));
	bkblock_acells(b, 0);
	return b;
}

void bkblock_push1(caryll_bkblock *b, bk_cell_type type, uint32_t x) {
	bk_cell *cell = bkblock_grow(b, 1);
	cell->t = type, cell->z = x;
	if (type >= p16) b->_leaf = false;
}

static void vbkpushitems(caryll_bkblock *b, bk_cell_type type0, va_list ap) {
	bk_cell_type curtype = type0;
	while (curtype) {
		uint32_t par = va_arg(ap, int);
		bkblock_push1(b, curtype, par);
		curtype = va_arg(ap, int);
	}
}

caryll_bkblock *new_bkblock(bk_cell_type type0, ...) {
	va_list ap;
	va_start(ap, type0);
	caryll_bkblock *b = _bkblock_init();
	vbkpushitems(b, type0, ap);
	va_end(ap);
	return b;
}

caryll_bkblock *bkblock_push(caryll_bkblock *b, bk_cell_type type0, ...) {
	va_list ap;
	va_start(ap, type0);
	vbkpushitems(b, type0, ap);
	va_end(ap);
	return b;
}
