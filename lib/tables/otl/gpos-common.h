#ifndef CARYLL_TABLES_OTL_GPOS_COMMON_H
#define CARYLL_TABLES_OTL_GPOS_COMMON_H

#include "otl.h"

// anchors
otl_anchor otl_anchor_absent();
otl_anchor otl_read_anchor(font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *otl_anchor_to_json(otl_anchor a);
otl_anchor otl_anchor_from_json(json_value *v);
caryll_bkblock *bkFromAnchor(otl_anchor a);

// mark arrays
void otl_delete_mark_array(otl_mark_array *array);
otl_mark_array *otl_read_mark_array(font_file_pointer data, uint32_t tableLength, uint32_t offset);

// position values
extern const uint8_t FORMAT_DX;
extern const uint8_t FORMAT_DY;
extern const uint8_t FORMAT_DWIDTH;
extern const uint8_t FORMAT_DHEIGHT;
extern const uint8_t bits_in[0x100];

uint8_t position_format_length(uint16_t format);
otl_position_value position_zero();
otl_position_value read_gpos_value(font_file_pointer data, uint32_t tableLength, uint32_t offset, uint16_t format);
uint8_t required_position_format(otl_position_value v);
void write_gpos_value(caryll_buffer *buf, otl_position_value v, uint16_t format);
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
int getPositon(otl_anchor anchor);
int byAnchorIndex(anchor_aggeration_hash *a, anchor_aggeration_hash *b);

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
