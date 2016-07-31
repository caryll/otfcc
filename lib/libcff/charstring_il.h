#ifndef CARYLL_CFF_CHARSTRING_IL
#define CARYLL_CFF_CHARSTRING_IL

#include "libcff.h"
#include <tables/glyf.h>

typedef enum {
	IL_ITEM_OPERAND,
	IL_ITEM_OPERATOR,
	IL_ITEM_SPECIAL,
	IL_ITEM_PROGID,
	IL_ITEM_PHANTOM_OPERATOR,
	IL_ITEM_PHANTOM_OPERAND
} il_type;

typedef struct {
	il_type type;
	uint16_t arity;
	union {
		double d;  // for type == IL_ITEM_OPERAND, IL_ITEM_PHANTOM_OPERAND
		int32_t i; // otherwise
	};
} charstring_instruction;

typedef struct {
	uint32_t length;
	uint32_t free;
	charstring_instruction *instr;
} charstring_il;

bool instruction_eq(charstring_instruction *z1, charstring_instruction *z2);

// basic ops
charstring_il *compile_glyph_to_il(glyf_glyph *g, uint16_t defaultWidth, uint16_t nominalWidth);
void glyph_il_peephole_optimization(charstring_il *il, caryll_options *options);
caryll_buffer *il2blob(charstring_il *il);

#endif
