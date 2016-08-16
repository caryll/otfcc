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
bool bk_cell_is_ptr(bk_cell *cell) {
	return cell->t >= p16;
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

void bkblock_pushint(caryll_bkblock *b, bk_cell_type type, uint32_t x) {
	bk_cell *cell = bkblock_grow(b, 1);
	cell->t = type;
	cell->z = x;
}
void bkblock_pushptr(caryll_bkblock *b, bk_cell_type type, caryll_bkblock *p) {
	bk_cell *cell = bkblock_grow(b, 1);
	cell->t = type;
	cell->p = p;
	b->_stem = true;
}

static void vbkpushitems(caryll_bkblock *b, bk_cell_type type0, va_list ap) {
	bk_cell_type curtype = type0;
	while (curtype) {
		if (curtype < p16) {
			uint32_t par = va_arg(ap, int);
			bkblock_pushint(b, curtype, par);
		} else {
			caryll_bkblock *par = va_arg(ap, caryll_bkblock *);
			bkblock_pushptr(b, curtype, par);
		}
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

caryll_bkblock *new_bkblock_from_buffer(/*MOVE*/ caryll_buffer *buf) {
	caryll_bkblock *b = new_bkblock(bkover);
	for (size_t j = 0; j < buf->size; j++) {
		bkblock_pushint(b, b8, buf->data[j]);
	}
	buffree(buf);
	return b;
}

void print_bkblock(caryll_bkblock *b) {
	fprintf(stderr, "Block size %08x\n", (uint32_t)b->length);
	fprintf(stderr, "------------------\n");
	for (uint32_t j = 0; j < b->length; j++) {
		if (bk_cell_is_ptr(b->cells + j)) {
			if (b->cells[j].p) {
				fprintf(stderr, "  %3d %p[%d]\n", b->cells[j].t, b->cells[j].p, b->cells[j].p->_index);
			} else {
				fprintf(stderr, "  %3d [NULL]\n", b->cells[j].t);
			}
		} else {
			fprintf(stderr, "  %3d %d\n", b->cells[j].t, b->cells[j].z);
		}
	}
	fprintf(stderr, "------------------\n");
}
