#include "charstring_il.h"

// Glyph building
static void ensureThereIsSpace(charstring_il *il) {
	if (il->free) return;
	il->free = 0x100;
	if (il->instr) {
		il->instr = realloc(il->instr, sizeof(charstring_instruction) * (il->length + il->free));
	} else {
		il->instr = malloc(sizeof(charstring_instruction) * (il->length + il->free));
	}
}

static void il_push_operand(charstring_il *il, float x) {
	ensureThereIsSpace(il);
	il->instr[il->length].type = IL_ITEM_OPERAND;
	il->instr[il->length].d = x;
	il->length++;
	il->free--;
}
static void il_push_special(charstring_il *il, int32_t s) {
	ensureThereIsSpace(il);
	il->instr[il->length].type = IL_ITEM_SPECIAL;
	il->instr[il->length].i = s;
	il->length++;
	il->free--;
}
static void il_push_op(charstring_il *il, int32_t op) {
	ensureThereIsSpace(il);
	il->instr[il->length].type = IL_ITEM_OPERATOR;
	il->instr[il->length].i = op;
	il->instr[il->length].arity = cs2_op_standard_arity(op);
	il->length++;
	il->free--;
}
static void il_lineto(charstring_il *il, float dx, float dy) {
	il_push_operand(il, dx);
	il_push_operand(il, dy);
	il_push_op(il, op_rlineto);
}
static void il_curveto(charstring_il *il, float dx1, float dy1, float dx2, float dy2, float dx3,
                       float dy3) {
	il_push_operand(il, dx1);
	il_push_operand(il, dy1);
	il_push_operand(il, dx2);
	il_push_operand(il, dy2);
	il_push_operand(il, dx3);
	il_push_operand(il, dy3);
	il_push_op(il, op_rrcurveto);
}

static void pushMasks(charstring_il *il, glyf_glyph *g, uint16_t points, uint16_t *jh,
                      uint16_t *jm) {
	while (*jm < g->numberOfContourMasks && g->contourMasks[*jm].pointsBefore <= points) {
		il_push_op(il, op_cntrmask);
		uint8_t maskByte = 0;
		uint8_t bits = 0;
		for (uint16_t j = 0; j < g->numberOfStemH; j++) {
			maskByte = maskByte << 1 | (g->contourMasks[*jm].maskH[j] & 1);
			bits += 1;
			if (bits == 8) {
				il_push_special(il, maskByte);
				bits = 0;
			}
		}
		for (uint16_t j = 0; j < g->numberOfStemV; j++) {
			maskByte = maskByte << 1 | (g->contourMasks[*jm].maskV[j] & 1);
			bits += 1;
			if (bits == 8) {
				il_push_special(il, maskByte);
				bits = 0;
			}
		}
		if (bits) {
			maskByte = maskByte << (8 - bits);
			il_push_special(il, maskByte);
		}
		*jm += 1;
	}
	while (*jh < g->numberOfHintMasks && g->hintMasks[*jh].pointsBefore <= points) {
		il_push_op(il, op_hintmask);
		uint8_t maskByte = 0;
		uint8_t bits = 0;
		for (uint16_t j = 0; j < g->numberOfStemH; j++) {
			maskByte = maskByte << 1 | (g->hintMasks[*jh].maskH[j] & 1);
			bits += 1;
			if (bits == 8) {
				il_push_special(il, maskByte);
				bits = 0;
			}
		}
		for (uint16_t j = 0; j < g->numberOfStemV; j++) {
			maskByte = maskByte << 1 | (g->hintMasks[*jh].maskV[j] & 1);
			bits += 1;
			if (bits == 8) {
				il_push_special(il, maskByte);
				bits = 0;
			}
		}
		if (bits) {
			maskByte = maskByte << (8 - bits);
			il_push_special(il, maskByte);
		}
		*jh += 1;
	}
}
static bool _pushStems(charstring_il *il, glyf_glyph *g) {
	bool hasmask =
	    (g->hintMasks && g->numberOfHintMasks) || (g->contourMasks && g->numberOfContourMasks);
	if (g->stemH && g->numberOfStemH) {
		float ref = 0;
		for (uint16_t j = 0; j < g->numberOfStemH; j++) {
			il_push_operand(il, g->stemH[j].position - ref);
			il_push_operand(il, g->stemH[j].width);
			ref = g->stemH[j].position + g->stemH[j].width;
		}
		if (hasmask) {
			il_push_op(il, op_hstemhm);
		} else {
			il_push_op(il, op_hstem);
		}
	}
	if (g->stemV && g->numberOfStemV) {
		float ref = 0;
		for (uint16_t j = 0; j < g->numberOfStemV; j++) {
			il_push_operand(il, g->stemV[j].position - ref);
			il_push_operand(il, g->stemV[j].width);
			ref = g->stemV[j].position + g->stemV[j].width;
		}
		if (hasmask) {
			il_push_op(il, op_vstemhm);
		} else {
			il_push_op(il, op_vstem);
		}
	}
	return hasmask;
}
charstring_il *compile_glyph_to_il(glyf_glyph *g, uint16_t defaultWidth, uint16_t nominalWidth) {
	charstring_il *il;
	NEW_CLEAN(il);
	// Convert absolute positions to deltas
	float x = 0;
	float y = 0;
	for (uint16_t c = 0; c < g->numberOfContours; c++) {
		glyf_contour *contour = &(g->contours[c]);
		uint16_t n = contour->pointsCount;
		for (uint16_t j = 0; j < n; j++) {
			float dx = contour->points[j].x - x;
			float dy = contour->points[j].y - y;
			x = contour->points[j].x, y = contour->points[j].y;
			contour->points[j].x = dx;
			contour->points[j].y = dy;
		}
	}

	// Write IL
	if (g->advanceWidth != defaultWidth) {
		il_push_operand(il, (int)(g->advanceWidth) - (int)(nominalWidth));
	}
	bool hasmask = _pushStems(il, g);
	uint16_t pointsSofar = 0;
	uint16_t jh = 0;
	uint16_t jm = 0;
	if (hasmask) pushMasks(il, g, pointsSofar, &jh, &jm);
	for (uint16_t c = 0; c < g->numberOfContours; c++) {
		glyf_contour *contour = &(g->contours[c]);
		uint16_t n = contour->pointsCount;
		if (n == 0) continue;
		il_push_operand(il, contour->points[0].x);
		il_push_operand(il, contour->points[0].y);
		il_push_op(il, op_rmoveto);
		pointsSofar++;
		if (hasmask) pushMasks(il, g, pointsSofar, &jh, &jm);
		for (uint16_t j = 1; j < n; j++) {
			if (contour->points[j].onCurve) {
				il_lineto(il, contour->points[j].x, contour->points[j].y);
				pointsSofar++;
			} else {
				if (j < n - 2 && !contour->points[j + 1].onCurve &&
				    contour->points[j + 2].onCurve) {
					il_curveto(il, contour->points[j].x, contour->points[j].y,
					           contour->points[j + 1].x, contour->points[j + 1].y,
					           contour->points[j + 2].x, contour->points[j + 2].y);
					pointsSofar += 3;
					j += 2;
				} else {
					il_lineto(il, contour->points[j].x, contour->points[j].y);
					pointsSofar++;
				}
			}
			if (hasmask) pushMasks(il, g, pointsSofar, &jh, &jm);
		}
	}
	il_push_op(il, op_endchar);
	return il;
}

// Pattern-based peephole optimization
static bool il_matchtype(charstring_il *il, uint32_t j, uint32_t k, il_type t) {
	if (k >= il->length) return false;
	for (uint32_t m = j; m < k; m++) {
		if (il->instr[m].type != t) return false;
	}
	return true;
}
static bool il_matchop(charstring_il *il, uint32_t j, int32_t op) {
	if (il->instr[j].type != IL_ITEM_OPERATOR) return false;
	if (il->instr[j].i != op) return false;
	return true;
}
static uint8_t zroll(charstring_il *il, uint32_t j, int32_t op, int32_t op2, ...) {
	uint8_t arity = cs2_op_standard_arity(op);
	if (arity > 16 || j + arity >= il->length) return 0;
	if ((j == 0 || // We are at the beginning of charstring
	     !il_matchtype(il, j - 1, j,
	                   IL_ITEM_PHANTOM_OPERATOR)) // .. or we are right after a solid operator
	    && il_matchop(il, j + arity, op)          // The next operator is <op>
	    && il_matchtype(il, j, j + arity, IL_ITEM_OPERAND) // And we have correct number of operands
	    ) {
		va_list ap;
		uint8_t check = true;
		uint8_t resultArity = arity;
		bool mask[16];
		va_start(ap, op2);
		for (uint32_t m = 0; m < arity; m++) {
			int checkzero = va_arg(ap, int);
			mask[m] = checkzero;
			if (checkzero) {
				resultArity -= 1;
				check = check && il->instr[j + m].d == 0;
			}
		}
		va_end(ap);
		if (check) {
			for (uint32_t m = 0; m < arity; m++) {
				if (mask[m]) { il->instr[j + m].type = IL_ITEM_PHANTOM_OPERAND; }
			}
			il->instr[j + arity].i = op2;
			il->instr[j + arity].arity = resultArity;
			return arity;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}
static uint8_t opop_roll(charstring_il *il, uint32_t j, int32_t op1, int32_t arity, int32_t op2,
                         int32_t resultop) {
	if (j + 1 + arity >= il->length) return 0;
	charstring_instruction *current = &(il->instr[j]);
	charstring_instruction *nextop = &(il->instr[j + 1 + arity]);
	if (il_matchop(il, j, op1)                                     // match this operator
	    && il_matchtype(il, j + 1, j + 1 + arity, IL_ITEM_OPERAND) // match operands
	    && il_matchop(il, j + 1 + arity, op2)                      // match next operator
	    && current->arity + nextop->arity <= type2_argument_stack  // stack is not full
	    ) {
		current->type = IL_ITEM_PHANTOM_OPERATOR;
		nextop->i = resultop;
		nextop->arity += current->arity;
		return arity + 1;
	} else {
		return 0;
	}
}
static uint8_t hvlineto_roll(charstring_il *il, uint32_t j) {
	if (j + 3 >= il->length) return 0;
	charstring_instruction *current = &(il->instr[j]);
	// We will check whether operand <checkdelta> is zero
	//          ODD     EVEN -- current arity
	// hlineto   X       Y
	// vlineto   Y       X
	uint32_t checkdelta = ((bool)(current->arity & 1) ^ (bool)(current->i == op_vlineto) ? 1 : 2);
	if ((il_matchop(il, j, op_hlineto) || il_matchop(il, j, op_vlineto)) // a hlineto/vlineto
	    && il_matchop(il, j + 3, op_rlineto)                             // followed by a lineto
	    && il_matchtype(il, j + 1, j + 3, IL_ITEM_OPERAND)               // have enough operands
	    && il->instr[j + checkdelta].d == 0                              // and it is a h/v
	    && current->arity + 1 <= type2_argument_stack // we have enough stack space
	    ) {
		il->instr[j + checkdelta].type = IL_ITEM_PHANTOM_OPERAND;
		il->instr[j].type = IL_ITEM_PHANTOM_OPERATOR;
		il->instr[j + 3].i = current->i;
		il->instr[j + 3].arity = current->arity + 1;
		return 3;
	} else {
		return 0;
	}
}
static uint8_t hvvhcurve_roll(charstring_il *il, uint32_t j) {
	if (!il_matchop(il, j, op_hvcurveto) && !il_matchop(il, j, op_vhcurveto)) return 0;
	charstring_instruction *current = &(il->instr[j]);
	// Exit in case of array not long enough or we have already ended
	if (j + 7 >= il->length || current->arity & 1) return 0;
	bool hvcase = (bool)((current->arity >> 2) & 1) ^ (bool)(current->i == op_hvcurveto);
	// We will check whether operand <checkdelta> is zero
	//            ODD     EVEN -- current arity divided by 4
	// hvcurveto   X       Y
	// vhcurveto   Y       X
	uint32_t checkdelta1 = hvcase ? 2 : 1;
	uint32_t checkdelta2 = hvcase ? 5 : 6;
	if (il_matchop(il, j + 7, op_rrcurveto)                // followed by a curveto
	    && il_matchtype(il, j + 1, j + 7, IL_ITEM_OPERAND) // have enough operands
	    && il->instr[j + checkdelta1].d == 0               // and it is a h/v
	    ) {
		if (il->instr[j + checkdelta2].d == 0 && current->arity + 4 <= type2_argument_stack) {
			// The Standard case
			il->instr[j + checkdelta1].type = IL_ITEM_PHANTOM_OPERAND;
			il->instr[j + checkdelta2].type = IL_ITEM_PHANTOM_OPERAND;
			il->instr[j].type = IL_ITEM_PHANTOM_OPERATOR;
			il->instr[j + 7].i = current->i;
			il->instr[j + 7].arity = current->arity + 4;
			return 7;
		} else if (current->arity + 5 <= type2_argument_stack) {
			// The trailing case
			il->instr[j + checkdelta1].type = IL_ITEM_PHANTOM_OPERAND;
			il->instr[j].type = IL_ITEM_PHANTOM_OPERATOR;
			il->instr[j + 7].i = current->i;
			il->instr[j + 7].arity = current->arity + 5;
			if (hvcase) {
				// Swap the last two operands because hvcurveto's trailing operand is in y-x order
				double t = il->instr[j + 5].d;
				il->instr[j + 5].d = il->instr[j + 6].d;
				il->instr[j + 6].d = t;
			}
			return 7;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}
static uint8_t hhvvcurve_roll(charstring_il *il, uint32_t j) {
	if (!il_matchop(il, j, op_hhcurveto) && !il_matchop(il, j, op_vvcurveto)) return 0;
	charstring_instruction *current = &(il->instr[j]);
	// Exit in case of array not long enough or we have already ended
	if (j + 7 >= il->length) return 0;
	bool hh = current->i == op_hhcurveto;
	uint32_t checkdelta1 = hh ? 2 : 1;
	uint32_t checkdelta2 = hh ? 6 : 5;
	if (il_matchop(il, j + 7, op_rrcurveto)                // followed by a curveto
	    && il_matchtype(il, j + 1, j + 7, IL_ITEM_OPERAND) // have enough operands
	    && il->instr[j + checkdelta1].d == 0               // and it is a h/v
	    && il->instr[j + checkdelta2].d == 0               // and it is a h/v
	    && current->arity + 4 <= type2_argument_stack) {
		il->instr[j + checkdelta1].type = IL_ITEM_PHANTOM_OPERAND;
		il->instr[j + checkdelta2].type = IL_ITEM_PHANTOM_OPERAND;
		il->instr[j].type = IL_ITEM_PHANTOM_OPERATOR;
		il->instr[j + 7].i = current->i;
		il->instr[j + 7].arity = current->arity + 4;
		return 7;
	} else {
		return 0;
	}
}
static uint32_t nextstop(charstring_il *il, uint32_t j) {
	uint32_t delta = 0;
	for (uint32_t k = 0; k < il->length && il->instr[k].type == IL_ITEM_OPERAND; k++) {
		delta += 1;
	}
	return delta;
}

void glyph_il_peephole_optimization(charstring_il *il) {
	uint32_t j = 0;
	while (j < il->length) {
		j += zroll(il, j, op_rlineto, op_hlineto, 0, 1)                    // rlineto -> hlineto
		     || zroll(il, j, op_rlineto, op_vlineto, 1, 0)                 // rlineto -> vlineto
		     || zroll(il, j, op_rmoveto, op_hmoveto, 0, 1)                 // rmoveto -> hmoveto
		     || zroll(il, j, op_rmoveto, op_vmoveto, 1, 0)                 // rmoveto -> vmoveto
		     || zroll(il, j, op_rrcurveto, op_hvcurveto, 0, 1, 0, 0, 1, 0) // rrcurveto->hvcurveto
		     || zroll(il, j, op_rrcurveto, op_vhcurveto, 1, 0, 0, 0, 0, 1) // rrcurveto->vhcurveto
		     || zroll(il, j, op_rrcurveto, op_hhcurveto, 0, 1, 0, 0, 0, 1) // rrcurveto->hhcurveto
		     || zroll(il, j, op_rrcurveto, op_vvcurveto, 1, 0, 0, 0, 1, 0) // rrcurveto->vvcurveto
		     || opop_roll(il, j, op_rrcurveto, 6, op_rrcurveto, op_rrcurveto) // rrcurveto roll
		     || opop_roll(il, j, op_rrcurveto, 2, op_rlineto, op_rcurveline)  // rcurveline roll
		     || opop_roll(il, j, op_rlineto, 6, op_rrcurveto, op_rlinecurve)  // rlinecurve roll
		     || opop_roll(il, j, op_rlineto, 2, op_rlineto, op_rlineto)       // rlineto roll
		     || hvlineto_roll(il, j)  // hlineto-vlineto roll
		     || hhvvcurve_roll(il, j) // hhcurveto-vvcurveto roll
		     || hvvhcurve_roll(il, j) // hvcurveto-vhcurveto roll
		     || nextstop(il, j)       // move to next stop
		     || 1;                    // nothing match
	}
}

// IL to buffer conversion
caryll_buffer *il2blob(charstring_il *il) {
	caryll_buffer *blob = bufnew();

	uint16_t stackDepth = 0;
	for (uint16_t j = 0; j < il->length; j++) {
		switch (il->instr[j].type) {
			case IL_ITEM_OPERAND: {
				merge_cs2_operand(blob, il->instr[j].d);
				stackDepth++;
				break;
			}
			case IL_ITEM_OPERATOR: {
				merge_cs2_operator(blob, il->instr[j].i);
				stackDepth = 0;
				break;
			}
			case IL_ITEM_SPECIAL: {
				merge_cs2_special(blob, il->instr[j].i);
				break;
			}
			default:
				break;
		}
	}
	return blob;
}
