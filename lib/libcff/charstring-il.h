#ifndef CARYLL_cff_CHARSTRING_IL
#define CARYLL_cff_CHARSTRING_IL

#include "libcff.h"
#include <tables/glyf.h>

typedef enum {
	IL_ITEM_OPERAND,
	IL_ITEM_OPERATOR,
	IL_ITEM_SPECIAL,
	IL_ITEM_PROGID,
	IL_ITEM_PHANTOM_OPERATOR,
	IL_ITEM_PHANTOM_OPERAND
} cff_InstructionType;

typedef struct {
	cff_InstructionType type;
	uint16_t arity;
	union {
		double d;  // for type == IL_ITEM_OPERAND, IL_ITEM_PHANTOM_OPERAND
		int32_t i; // otherwise
	};
} cff_CharstringInstruction;

typedef struct {
	uint32_t length;
	uint32_t free;
	cff_CharstringInstruction *instr;
} cff_CharstringIL;

bool instruction_eq(cff_CharstringInstruction *z1, cff_CharstringInstruction *z2);

// basic ops
cff_CharstringIL *cff_compileGlyphToIL(glyf_Glyph *g, uint16_t defaultWidth, uint16_t nominalWidth);
void cff_optimizeIL(cff_CharstringIL *il, const caryll_Options *options);
caryll_buffer *cff_build_IL(cff_CharstringIL *il);

#endif
