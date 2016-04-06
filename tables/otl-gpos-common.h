#ifndef CARYLL_TABLES_OTL_GPOS_COMMON_H
#define CARYLL_TABLES_OTL_GPOS_COMMON_H

#include "otl.h"

// anchors
otl_anchor otl_anchor_absent();
otl_anchor otl_read_anchor(font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *otl_anchor_to_json(otl_anchor a);
otl_anchor otl_anchor_from_json(json_value *v);

// mark arrays
void otl_delete_mark_array(otl_mark_array *array);
otl_mark_array *otl_read_mark_array(font_file_pointer data, uint32_t tableLength, uint32_t offset);

// position values

#define a(n) n + 0, n + 1, n + 1, n + 2             // 2 bits
#define b(n) a(n + 0), a(n + 1), a(n + 1), a(n + 2) // 4 bits
#define c(n) b(n + 0), b(n + 1), b(n + 1), b(n + 2) // 6 bits
#define d(n) c(n + 0), c(n + 1), c(n + 1), c(n + 2) // 8 bits
static const uint8_t bits_in[0x100] = {d(0)};
#undef d
#undef c
#undef b
#undef a

static INLINE uint8_t position_format_length(uint16_t format) { return bits_in[format & 0xFF] << 1; }

static INLINE otl_position_value otl_read_position(font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                                   uint16_t format) {
	otl_position_value v = {0, 0, 0, 0};
	if (tableLength < offset + position_format_length(format)) return v;
	if (format & 1) { v.dx = read_16u(data + offset), offset += 2; };
	if (format & 2) { v.dy = read_16u(data + offset), offset += 2; };
	if (format & 4) { v.dWidth = read_16u(data + offset), offset += 2; };
	if (format & 8) { v.dHeight = read_16u(data + offset), offset += 2; };
	return v;
}

static INLINE uint8_t required_position_format(otl_position_value v) {
	return (v.dx ? 1 : 0) | (v.dy ? 2 : 0) | (v.dWidth ? 4 : 0) | (v.dHeight ? 8 : 0);
}
static INLINE void otl_write_position(caryll_buffer *buf, otl_position_value v, uint16_t format) {
	if (format & 1) bufwrite16b(buf, v.dx);
	if (format & 2) bufwrite16b(buf, v.dy);
	if (format & 4) bufwrite16b(buf, v.dWidth);
	if (format & 8) bufwrite16b(buf, v.dHeight);
}
json_value *gpos_value_to_json(otl_position_value value);
otl_position_value gpos_value_from_json(json_value *pos);

// anchor aggerator
typedef struct {
	int position;
	int x;
	int y;
	uint16_t index;
	UT_hash_handle hh;
} anchor_aggeration_hash;
static INLINE int getPositon(otl_anchor anchor) { return ((uint16_t)anchor.x) << 16 | ((uint16_t)anchor.y); }
static INLINE int byAnchorIndex(anchor_aggeration_hash *a, anchor_aggeration_hash *b) { return a->index - b->index; }

#define ANCHOR_AGGERATOR_PUSH(agh, anchor)                                                                             \
	if ((anchor).present) {                                                                                            \
		anchor_aggeration_hash *s;                                                                                     \
		int position = getPositon(anchor);                                                                             \
		HASH_FIND_INT((agh), &position, s);                                                                            \
		if (!s) {                                                                                                      \
			NEW(s);                                                                                                    \
			s->position = position;                                                                                    \
			s->x = (anchor).x;                                                                                         \
			s->y = (anchor).y;                                                                                         \
			s->index = HASH_COUNT(agh);                                                                                \
			HASH_ADD_INT(agh, position, s);                                                                            \
		}                                                                                                              \
	}

#endif
