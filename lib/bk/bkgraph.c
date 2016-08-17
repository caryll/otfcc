#include "bkgraph.h"

static bkgraph_entry *_bkgraph_grow(caryll_bkgraph *f) {
	if (f->free) {
		f->length++;
		f->free--;
	} else {
		f->length = f->length + 1;
		f->free = (f->length >> 1) & 0xFFFFFF;
		if (f->entries) {
			f->entries = realloc(f->entries, (f->length + f->free) * sizeof(bkgraph_entry));
		} else {
			f->entries = malloc((f->length + f->free) * sizeof(bkgraph_entry));
		}
	}
	return &(f->entries[f->length - 1]);
}

static uint32_t dfs_insert_cells(caryll_bkblock *b, caryll_bkgraph *f, uint32_t *order) {
	if (!b || b->_visitstate == VISIT_GRAY) return 0;
	if (b->_visitstate == VISIT_BLACK) return b->_height;
	b->_visitstate = VISIT_GRAY;
	uint32_t height = 0;
	for (uint32_t j = 0; j < b->length; j++) {
		bk_cell *cell = &(b->cells[j]);
		if (bk_cell_is_ptr(cell) && cell->p) {
			uint32_t thatHeight = dfs_insert_cells(cell->p, f, order);
			if (thatHeight + 1 > height) height = thatHeight + 1;
		}
	}
	bkgraph_entry *e = _bkgraph_grow(f);
	e->alias = 0;
	e->block = b;
	*order += 1;
	e->order = *order;
	e->height = b->_height = height;
	b->_visitstate = VISIT_BLACK;
	return height;
}

static int _by_order(const void *_a, const void *_b) {
	const bkgraph_entry *a = _a;
	const bkgraph_entry *b = _b;
	return a->height == b->height ? a->order - b->order : b->height - a->height;
}

caryll_bkgraph *caryll_bkgraph_from_block(caryll_bkblock *b) {
	caryll_bkgraph *forest = calloc(1, sizeof(caryll_bkgraph));
	uint32_t tsOrder = 0;
	dfs_insert_cells(b, forest, &tsOrder);
	qsort(forest->entries, forest->length, sizeof(bkgraph_entry), _by_order);
	for (uint32_t j = 0; j < forest->length; j++) {
		forest->entries[j].block->_index = j;
		forest->entries[j].alias = j;
	}
	return forest;
}

void caryll_delete_bkgraph(caryll_bkgraph *f) {
	if (!f || !f->entries) return;
	for (uint32_t j = 0; j < f->length; j++) {
		caryll_bkblock *b = f->entries[j].block;
		if (b && b->cells) free(b->cells);
		free(b);
	}
	free(f->entries);
}
static size_t caryll_bkblock_size(caryll_bkblock *b) {
	size_t size = 0;
	for (uint32_t j = 0; j < b->length; j++)
		switch (b->cells[j].t) {
			case b8:
				size += 1;
				break;
			case b16:
			case p16:
				size += 2;
				break;
			case b32:
			case p32:
				size += 4;
				break;
			default:
				break;
		}
	return size;
}

static uint32_t getoffset(size_t *offsets, caryll_bkblock *ref, caryll_bkblock *target, uint8_t bits) {
	size_t offref = offsets[ref->_index];
	size_t offtgt = offsets[target->_index];
	if (offtgt < offref || (offtgt - offref) >> bits) {
		fprintf(stderr, "[otfcc-fea] Warning : Unable to fit offset %d into %d bits.\n", (int32_t)(offtgt - offref),
		        bits);
	}
	return (uint32_t)(offtgt - offref);
}
static void caryll_write_bkblock(caryll_buffer *buf, caryll_bkblock *b, size_t *offsets) {
	for (uint32_t j = 0; j < b->length; j++) {
		switch (b->cells[j].t) {
			case b8:
				bufwrite8(buf, b->cells[j].z);
				break;
			case b16:
				bufwrite16b(buf, b->cells[j].z);
				break;
			case b32:
				bufwrite32b(buf, b->cells[j].z);
				break;
			case p16:
				if (b->cells[j].p) {
					bufwrite16b(buf, getoffset(offsets, b, b->cells[j].p, 16));
				} else {
					bufwrite16b(buf, 0);
				}
				break;
			case p32:
				if (b->cells[j].p) {
					bufwrite32b(buf, getoffset(offsets, b, b->cells[j].p, 32));
				} else {
					bufwrite32b(buf, 0);
				}
				break;
			default:
				break;
		}
	}
}

static uint32_t gethash(caryll_bkblock *b) {
	uint32_t h = 5381;
	for (uint32_t j = 0; j < b->length; j++) {
		h = ((h << 5) + h) + b->cells[j].t;
		h = ((h << 5) + h);
		switch (b->cells[j].t) {
			case b8:
			case b16:
			case b32:
				h += b->cells[j].z;
				break;
			case p16:
			case p32:
				if (b->cells[j].p) { h += b->cells[j].p->_index; }
				break;
			default:
				break;
		}
	}
	return h;
}

static bool compareblock(caryll_bkblock *a, caryll_bkblock *b) {
	if (!a && !b) return true;
	if (!a || !b) return false;
	if (a->length != b->length) return false;
	for (uint16_t j = 0; j < a->length; j++) {
		if (a->cells[j].t != b->cells[j].t) return false;
		switch (a->cells[j].t) {
			case b8:
			case b16:
			case b32:
				if (a->cells[j].z != b->cells[j].z) return false;
				break;
			case p16:
			case p32:
				if (a->cells[j].p != b->cells[j].p) return false;
				break;
			default:
				break;
		}
	}
	return true;
}
static bool compareEntry(bkgraph_entry *a, bkgraph_entry *b) {
	if (a->hash != b->hash) return false;
	return compareblock(a->block, b->block);
}

static void replaceptr(caryll_bkgraph *f, caryll_bkblock *b) {
	for (uint32_t j = 0; j < b->length; j++) {
		switch (b->cells[j].t) {
			case p16:
			case p32:
				if (b->cells[j].p) {
					uint32_t index = b->cells[j].p->_index;
					while (f->entries[index].alias != index) {
						index = f->entries[index].alias;
					}
					b->cells[j].p = f->entries[index].block;
				}
				break;
			default:
				break;
		}
	}
}

void caryll_minimize_bkgraph(caryll_bkgraph *f) {
	uint32_t rear = (uint32_t)(f->length - 1);
	while (rear > 0) {
		uint32_t front = rear;
		while (f->entries[front].height == f->entries[rear].height && front > 0) {
			front--;
		}
		front++;
		for (uint32_t j = front; j <= rear; j++) {
			f->entries[j].hash = gethash(f->entries[j].block);
		}
		for (uint32_t j = front; j <= rear; j++) {
			bkgraph_entry *a = &(f->entries[j]);
			if (a->alias == j) {
				for (uint32_t k = j + 1; k <= rear; k++) {
					bkgraph_entry *b = &(f->entries[k]);
					if (b->alias == k && compareEntry(a, b)) { b->alias = j; }
				}
			}
		}
		// replace pointers with aliased
		for (uint32_t j = 0; j < front; j++) {
			replaceptr(f, f->entries[j].block);
		}
		rear = front - 1;
	}
}

caryll_buffer *caryll_write_bkgraph(caryll_bkgraph *f) {
	caryll_buffer *buf = bufnew();
	size_t *offsets = calloc(f->length + 1, sizeof(size_t));
	offsets[0] = 0;
	size_t validEncs = 0;
	for (uint32_t j = 0; j < f->length; j++) {
		if (f->entries[j].alias == j) {
			offsets[j + 1] = offsets[j] + caryll_bkblock_size(f->entries[j].block);
			validEncs += 1;
		} else {
			offsets[j + 1] = offsets[j];
		}
	}
	for (uint32_t j = 0; j < f->length; j++)
		if (f->entries[j].alias == j) { caryll_write_bkblock(buf, f->entries[j].block, offsets); }
	return buf;
}

caryll_buffer *caryll_write_bk(/*MOVE*/ caryll_bkblock *root) {
	caryll_bkgraph *f = caryll_bkgraph_from_block(root);
	caryll_minimize_bkgraph(f);
	caryll_buffer *buf = caryll_write_bkgraph(f);
	caryll_delete_bkgraph(f);
	return buf;
}
