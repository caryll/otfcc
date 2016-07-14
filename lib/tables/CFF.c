#include "CFF.h"

const float DEFAULT_BLUE_SCALE = 0.039625;
const float DEFAULT_BLUE_SHIFT = 7;
const float DEFAULT_BLUE_FUZZ = 1;
const float DEFAULT_EXPANSION_FACTOR = 0.06;

static cff_private *caryll_new_CFF_private() {
	cff_private *pd = NULL;
	NEW_CLEAN(pd);
	pd->blueFuzz = DEFAULT_BLUE_FUZZ;
	pd->blueScale = DEFAULT_BLUE_SCALE;
	pd->blueShift = DEFAULT_BLUE_SHIFT;
	pd->expansionFactor = DEFAULT_EXPANSION_FACTOR;
	return pd;
}
table_CFF *caryll_new_CFF() {
	table_CFF *table = NULL;
	NEW_CLEAN(table);
	table->underlinePosition = -100;
	table->underlineThickness = 50;
	return table;
}

static void caryll_delete_privatedict(cff_private *priv) {
	if (!priv) return;
	FREE(priv->blueValues);
	FREE(priv->otherBlues);
	FREE(priv->familyBlues);
	FREE(priv->familyOtherBlues);
	FREE(priv->stemSnapH);
	FREE(priv->stemSnapV);
	FREE(priv);
}
void caryll_delete_CFF(table_CFF *table) {
	if (!table) return;
	sdsfree(table->version);
	sdsfree(table->notice);
	sdsfree(table->copyright);
	sdsfree(table->fullName);
	sdsfree(table->familyName);
	sdsfree(table->weight);

	sdsfree(table->fontName);
	sdsfree(table->cidRegistry);
	sdsfree(table->cidOrdering);

	FREE(table->fontMatrix);
	caryll_delete_privatedict(table->privateDict);

	if (table->fdArray) {
		for (uint16_t j = 0; j < table->fdArrayCount; j++) { caryll_delete_CFF(table->fdArray[j]); }
		free(table->fdArray);
	}
}

typedef struct {
	int32_t fdArrayIndex;
	table_CFF *meta;
	table_glyf *glyphs;
	CFF_File *cffFile;
	uint64_t seed;
} cff_parse_context;
static void callback_extract_private(uint32_t op, uint8_t top, CFF_Value *stack, void *_context) {
	cff_parse_context *context = (cff_parse_context *)_context;
	table_CFF *meta = context->meta;
	if (context->fdArrayIndex >= 0 && context->fdArrayIndex < meta->fdArrayCount) {
		meta = meta->fdArray[context->fdArrayIndex];
	}
	cff_private *pd = meta->privateDict;
	switch (op) {
		// DELTAs
		case op_BlueValues: {
			pd->blueValuesCount = top;
			NEW_N(pd->blueValues, pd->blueValuesCount);
			for (uint16_t j = 0; j < pd->blueValuesCount; j++) {
				pd->blueValues[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_OtherBlues: {
			pd->otherBluesCount = top;
			NEW_N(pd->otherBlues, pd->otherBluesCount);
			for (uint16_t j = 0; j < pd->otherBluesCount; j++) {
				pd->otherBlues[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_FamilyBlues: {
			pd->familyBluesCount = top;
			NEW_N(pd->familyBlues, pd->familyBluesCount);
			for (uint16_t j = 0; j < pd->familyBluesCount; j++) {
				pd->familyBlues[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_FamilyOtherBlues: {
			pd->familyOtherBluesCount = top;
			NEW_N(pd->familyOtherBlues, pd->familyOtherBluesCount);
			for (uint16_t j = 0; j < pd->familyOtherBluesCount; j++) {
				pd->familyOtherBlues[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_StemSnapH: {
			pd->stemSnapHCount = top;
			NEW_N(pd->stemSnapH, pd->stemSnapHCount);
			for (uint16_t j = 0; j < pd->stemSnapHCount; j++) {
				pd->stemSnapH[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_StemSnapV: {
			pd->stemSnapVCount = top;
			NEW_N(pd->stemSnapV, pd->stemSnapVCount);
			for (uint16_t j = 0; j < pd->stemSnapVCount; j++) {
				pd->stemSnapV[j] = cffnum(stack[j]);
			}
			break;
		}
		// Numbers
		case op_BlueScale:
			if (top) { pd->blueScale = cffnum(stack[top - 1]); }
			break;
		case op_BlueShift:
			if (top) { pd->blueShift = cffnum(stack[top - 1]); }
			break;
		case op_BlueFuzz:
			if (top) { pd->blueFuzz = cffnum(stack[top - 1]); }
			break;
		case op_StdHW:
			if (top) { pd->stdHW = cffnum(stack[top - 1]); }
			break;
		case op_StdVW:
			if (top) { pd->stdVW = cffnum(stack[top - 1]); }
			break;
		case op_ForceBold:
			if (top) { pd->forceBold = cffnum(stack[top - 1]); }
			break;
		case op_LanguageGroup:
			if (top) { pd->languageGroup = cffnum(stack[top - 1]); }
			break;
		case op_ExpansionFactor:
			if (top) { pd->expansionFactor = cffnum(stack[top - 1]); }
			break;
		case op_initialRandomSeed:
			if (top) { pd->initialRandomSeed = cffnum(stack[top - 1]); }
			break;
		case op_defaultWidthX:
			if (top) { pd->defaultWidthX = cffnum(stack[top - 1]); }
			break;
		case op_nominalWidthX:
			if (top) { pd->nominalWidthX = cffnum(stack[top - 1]); }
			break;
	}
}

static void callback_extract_fd(uint32_t op, uint8_t top, CFF_Value *stack, void *_context) {
	cff_parse_context *context = (cff_parse_context *)_context;
	CFF_File *file = context->cffFile;
	table_CFF *meta = context->meta;
	if (context->fdArrayIndex >= 0 && context->fdArrayIndex < meta->fdArrayCount) {
		meta = meta->fdArray[context->fdArrayIndex];
	}
	switch (op) {
		case op_version:
			if (top) { meta->version = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_Notice:
			if (top) { meta->notice = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_Copyright:
			if (top) { meta->copyright = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_FontName:
			if (top) { meta->fontName = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_FullName:
			if (top) { meta->fullName = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_FamilyName:
			if (top) { meta->familyName = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_Weight:
			if (top) { meta->weight = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_FontBBox:
			if (top >= 4) {
				meta->fontBBoxLeft = cffnum(stack[top - 4]);
				meta->fontBBoxBottom = cffnum(stack[top - 3]);
				meta->fontBBoxRight = cffnum(stack[top - 2]);
				meta->fontBBoxTop = cffnum(stack[top - 1]);
			}
			break;
		case op_FontMatrix:
			if (top >= 6) {
				NEW(meta->fontMatrix);
				meta->fontMatrix->a = cffnum(stack[top - 6]);
				meta->fontMatrix->b = cffnum(stack[top - 5]);
				meta->fontMatrix->c = cffnum(stack[top - 4]);
				meta->fontMatrix->d = cffnum(stack[top - 3]);
				meta->fontMatrix->x = cffnum(stack[top - 2]);
				meta->fontMatrix->y = cffnum(stack[top - 1]);
			}
			break;
		case op_isFixedPitch:
			if (top) { meta->isFixedPitch = (bool)cffnum(stack[top - 1]); }
			break;
		case op_ItalicAngle:
			if (top) { meta->italicAngle = cffnum(stack[top - 1]); }
			break;
		case op_UnderlinePosition:
			if (top) { meta->underlinePosition = cffnum(stack[top - 1]); }
			break;
		case op_UnderlineThickness:
			if (top) { meta->underlineThickness = cffnum(stack[top - 1]); }
			break;
		case op_StrokeWidth:
			if (top) { meta->strokeWidth = cffnum(stack[top - 1]); }
			break;

		// Private
		case op_Private:
			if (top >= 2) {
				uint32_t privateLength = cffnum(stack[top - 2]);
				uint32_t privateOffset = cffnum(stack[top - 1]);
				meta->privateDict = caryll_new_CFF_private();
				cff_dict_callback(file->raw_data + privateOffset, privateLength, context,
				                  callback_extract_private);
			}
			break;
		// CID
		case op_ROS:
			if (top >= 3) {
				meta->isCID = true;
				meta->cidRegistry = sdsget_cff_sid(stack[top - 3].i, file->string);
				meta->cidOrdering = sdsget_cff_sid(stack[top - 2].i, file->string);
				meta->cidSupplement = cffnum(stack[top - 1]);
			}
			break;
	}
}

typedef struct {
	glyf_glyph *g;
	uint16_t jContour;
	uint16_t jPoint;
	float defaultWidthX;
	float nominalWidthX;
	uint16_t pointsDefined;
	uint8_t definedHStems;
	uint8_t definedVStems;
	uint8_t definedHintMasks;
	uint8_t definedContourMasks;
	uint64_t randx;
} outline_builder_context;

static void callback_count_contour(void *context) {
	((outline_builder_context *)context)->g->numberOfContours += 1;
}
static void callback_countpoint_next_contour(void *_context) {
	outline_builder_context *context = (outline_builder_context *)_context;
	context->jContour += 1;
	context->g->contours[context->jContour - 1].pointsCount = 0;
	context->jPoint = 0;
}
static void callback_countpoint_lineto(void *_context, float x1, float y1) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (context->jContour) {
		context->g->contours[context->jContour - 1].pointsCount += 1;
		context->jPoint += 1;
	}
}
static void callback_countpoint_curveto(void *_context, float x1, float y1, float x2, float y2,
                                        float x3, float y3) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (context->jContour) {
		context->g->contours[context->jContour - 1].pointsCount += 3;
		context->jPoint += 3;
	}
}
static void callback_countpoint_sethint(void *_context, bool isVertical, float position,
                                        float width) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (isVertical) {
		context->g->numberOfStemV += 1;
	} else {
		context->g->numberOfStemH += 1;
	}
}
static void callback_countpoint_setmask(void *_context, bool isContourMask, bool *mask) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (isContourMask) {
		context->g->numberOfContourMasks += 1;
	} else {
		context->g->numberOfHintMasks += 1;
	}
	free(mask);
}

static void callback_draw_setwidth(void *_context, float width) {
	outline_builder_context *context = (outline_builder_context *)_context;
	context->g->advanceWidth = width + context->nominalWidthX;
}
static void callback_draw_next_contour(void *_context) {
	outline_builder_context *context = (outline_builder_context *)_context;
	context->jContour += 1;
	context->jPoint = 0;
}
static void callback_draw_lineto(void *_context, float x1, float y1) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (context->jContour) {
		context->g->contours[context->jContour - 1].points[context->jPoint].onCurve = true;
		context->g->contours[context->jContour - 1].points[context->jPoint].x = x1;
		context->g->contours[context->jContour - 1].points[context->jPoint].y = y1;
		context->jPoint += 1;
		context->pointsDefined += 1;
	}
}
static void callback_draw_curveto(void *_context, float x1, float y1, float x2, float y2, float x3,
                                  float y3) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (context->jContour) {
		context->g->contours[context->jContour - 1].points[context->jPoint].onCurve = false;
		context->g->contours[context->jContour - 1].points[context->jPoint].x = x1;
		context->g->contours[context->jContour - 1].points[context->jPoint].y = y1;
		context->g->contours[context->jContour - 1].points[context->jPoint + 1].onCurve = false;
		context->g->contours[context->jContour - 1].points[context->jPoint + 1].x = x2;
		context->g->contours[context->jContour - 1].points[context->jPoint + 1].y = y2;
		context->g->contours[context->jContour - 1].points[context->jPoint + 2].onCurve = true;
		context->g->contours[context->jContour - 1].points[context->jPoint + 2].x = x3;
		context->g->contours[context->jContour - 1].points[context->jPoint + 2].y = y3;
		context->jPoint += 3;
		context->pointsDefined += 3;
	}
}
static void callback_draw_sethint(void *_context, bool isVertical, float position, float width) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (isVertical) {
		context->g->stemV[context->definedVStems].position = position;
		context->g->stemV[context->definedVStems].width = width;
		context->definedVStems += 1;
	} else {
		context->g->stemH[context->definedHStems].position = position;
		context->g->stemH[context->definedHStems].width = width;
		context->definedHStems += 1;
	}
}
static void callback_draw_setmask(void *_context, bool isContourMask, bool *maskArray) {
	outline_builder_context *context = (outline_builder_context *)_context;
	uint16_t maskIndex = isContourMask ? context->definedContourMasks : context->definedHintMasks;
	glyf_postscript_hint_mask *maskList =
	    isContourMask ? context->g->contourMasks : context->g->hintMasks;
	glyf_postscript_hint_mask *mask;
	bool duplicateMask = false;

	if (maskIndex > 0) {
		glyf_postscript_hint_mask *lastMask = &(maskList)[maskIndex - 1];
		if (lastMask->pointsBefore == context->pointsDefined) {
			mask = lastMask;
			duplicateMask = true;
		} else {
			mask = &(maskList)[maskIndex];
		}
	} else {
		mask = &(maskList)[maskIndex];
	}
	mask->pointsBefore = context->pointsDefined;

	for (uint16_t j = 0; j < 0x100; j++) {
		mask->maskH[j] = j < context->g->numberOfStemH ? maskArray[j] : 0;
		mask->maskV[j] =
		    j < context->g->numberOfStemV ? maskArray[j + context->g->numberOfStemH] : 0;
	}

	free(maskArray);
	if (duplicateMask) return;
	if (isContourMask) {
		context->definedContourMasks += 1;
	} else {
		context->definedHintMasks += 1;
	}
}

static double callback_draw_getrand(void *_context) {
	// xorshift64* PRNG to double53
	outline_builder_context *context = (outline_builder_context *)_context;
	uint64_t x = context->randx;
	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	context->randx = x;
	union {
		uint64_t u;
		double d;
	} a;
	a.u = x * UINT64_C(2685821657736338717);
	a.u = (a.u >> 12) | UINT64_C(0x3FF0000000000000);
	double q = (a.u & 2048) ? (1.0 - (2.2204460492503131E-16 / 2.0)) : 1.0;
	return a.d - q;
}

static void buildOutline(uint16_t i, cff_parse_context *context) {
	CFF_File *f = context->cffFile;
	glyf_glyph *g = caryll_new_glyf_glyph();
	context->glyphs->glyphs[i] = g;
	uint64_t seed = context->seed;

	CFF_INDEX localSubrs;
	CFF_Stack stack;

	stack.index = 0;
	stack.stem = 0;

	outline_builder_context bc = {g, 0, 0, 0.0, 0.0, 0, 0, 0, 0, 0, 0};
	cff_outline_builder_interface pass1 = {NULL, callback_count_contour, NULL, NULL, NULL, NULL,
	                                       NULL};
	cff_outline_builder_interface pass2 = {NULL,
	                                       callback_countpoint_next_contour,
	                                       callback_countpoint_lineto,
	                                       callback_countpoint_curveto,
	                                       callback_countpoint_sethint,
	                                       callback_countpoint_setmask,
	                                       NULL};
	cff_outline_builder_interface pass3 = {callback_draw_setwidth, callback_draw_next_contour,
	                                       callback_draw_lineto,   callback_draw_curveto,
	                                       callback_draw_sethint,  callback_draw_setmask,
	                                       callback_draw_getrand};

	uint8_t fd = 0;
	if (f->fdselect.t != CFF_FDSELECT_UNSPECED)
		fd = parse_subr(i, f->raw_data, f->font_dict, f->fdselect, &localSubrs);
	else
		fd = parse_subr(i, f->raw_data, f->top_dict, f->fdselect, &localSubrs);

	g->fdSelectIndex = fd;
	if (context->meta->fdArray && fd >= 0 && fd < context->meta->fdArrayCount &&
	    context->meta->fdArray[fd]->privateDict) {
		bc.defaultWidthX = context->meta->fdArray[fd]->privateDict->defaultWidthX;
		bc.nominalWidthX = context->meta->fdArray[fd]->privateDict->nominalWidthX;
	} else if (context->meta->privateDict) {
		bc.defaultWidthX = context->meta->privateDict->defaultWidthX;
		bc.nominalWidthX = context->meta->privateDict->nominalWidthX;
	}
	g->advanceWidth = bc.defaultWidthX;

	uint8_t *charStringPtr = f->char_strings.data + f->char_strings.offset[i] - 1;
	uint32_t charStringLength = f->char_strings.offset[i + 1] - f->char_strings.offset[i];

	// PASS 1 : Count contours
	bc.randx = seed;
	parse_outline_callback(charStringPtr, charStringLength, f->global_subr, localSubrs, &stack, &bc,
	                       pass1);
	NEW_N(g->contours, g->numberOfContours);

	// PASS 2 : Count points
	stack.index = 0;
	stack.stem = 0;
	bc.randx = seed;
	parse_outline_callback(charStringPtr, charStringLength, f->global_subr, localSubrs, &stack, &bc,
	                       pass2);
	for (uint16_t j = 0; j < g->numberOfContours; j++) {
		NEW_N(g->contours[j].points, g->contours[j].pointsCount);
	}
	NEW_N(g->stemH, g->numberOfStemH);
	NEW_N(g->stemV, g->numberOfStemV);
	NEW_N(g->hintMasks, g->numberOfHintMasks);
	NEW_N(g->contourMasks, g->numberOfContourMasks);

	// PASS 3 : Draw points
	stack.index = 0;
	stack.stem = 0;
	bc.jContour = 0;
	bc.jPoint = 0;
	bc.randx = seed;
	parse_outline_callback(charStringPtr, charStringLength, f->global_subr, localSubrs, &stack, &bc,
	                       pass3);
	g->numberOfContourMasks = bc.definedContourMasks;
	g->numberOfHintMasks = bc.definedHintMasks;

	// PASS 4 : Turn deltas into absolute coordinates
	float cx = 0;
	float cy = 0;
	for (uint16_t j = 0; j < g->numberOfContours; j++) {
		for (uint16_t k = 0; k < g->contours[j].pointsCount; k++) {
			cx += g->contours[j].points[k].x;
			g->contours[j].points[k].x = cx;
			cy += g->contours[j].points[k].y;
			g->contours[j].points[k].y = cy;
		}
	}
	esrap_index(localSubrs);
	context->seed = bc.randx;
}

static void nameGlyphsAccordingToCFF(cff_parse_context *context) {
	if (context->meta->isCID) return;
	CFF_File *cffFile = context->cffFile;
	table_glyf *glyphs = context->glyphs;
	switch (cffFile->charsets.t) {
		case CFF_CHARSET_FORMAT0: {
			for (uint16_t j = 0; j < cffFile->charsets.s; j++) {
				uint16_t sid = cffFile->charsets.f0.glyph[j];
				sds glyphname = sdsget_cff_sid(sid, cffFile->string);
				if (glyphname) { glyphs->glyphs[j + 1]->name = glyphname; }
			}
			break;
		}
		case CFF_CHARSET_FORMAT1: {
			uint32_t glyphsNamedSofar = 1;
			for (uint16_t j = 0; j < cffFile->charsets.s; j++) {
				uint16_t first = cffFile->charsets.f1.range1[j].first;
				sds glyphname = sdsget_cff_sid(first, cffFile->string);
				if (glyphsNamedSofar < glyphs->numberGlyphs && glyphname) {
					glyphs->glyphs[glyphsNamedSofar]->name = glyphname;
				}
				glyphsNamedSofar++;
				for (uint16_t k = 0; k < cffFile->charsets.f1.range1[j].nleft; k++) {
					uint16_t sid = first + k + 1;
					sds glyphname = sdsget_cff_sid(sid, cffFile->string);
					if (glyphsNamedSofar < glyphs->numberGlyphs && glyphname) {
						glyphs->glyphs[glyphsNamedSofar]->name = glyphname;
					}
					glyphsNamedSofar++;
				}
			}
			break;
		}
		case CFF_CHARSET_FORMAT2: {
			uint32_t glyphsNamedSofar = 1;
			for (uint16_t j = 0; j < cffFile->charsets.s; j++) {
				uint16_t first = cffFile->charsets.f2.range2[j].first;
				sds glyphname = sdsget_cff_sid(first, cffFile->string);
				if (glyphsNamedSofar < glyphs->numberGlyphs && glyphname) {
					glyphs->glyphs[glyphsNamedSofar]->name = glyphname;
				}
				glyphsNamedSofar++;
				for (uint16_t k = 0; k < cffFile->charsets.f2.range2[j].nleft; k++) {
					uint16_t sid = first + k + 1;
					sds glyphname = sdsget_cff_sid(sid, cffFile->string);
					if (glyphsNamedSofar < glyphs->numberGlyphs && glyphname) {
						glyphs->glyphs[glyphsNamedSofar]->name = glyphname;
					}
					glyphsNamedSofar++;
				}
			}
			break;
		}
	}
}

caryll_cff_parse_result caryll_read_CFF_and_glyf(caryll_packet packet) {
	caryll_cff_parse_result ret;
	ret.meta = NULL;
	ret.glyphs = NULL;

	cff_parse_context context;
	context.fdArrayIndex = -1;
	context.meta = NULL;
	context.glyphs = NULL;
	context.cffFile = NULL;
	FOR_TABLE('CFF ', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		CFF_File *cffFile = CFF_stream_open(data, length);
		context.cffFile = cffFile;
		context.meta = caryll_new_CFF();

		// Extract data in TOP DICT
		cff_dict_callback(cffFile->top_dict.data,
		                  cffFile->top_dict.offset[1] - cffFile->top_dict.offset[0], &context,
		                  callback_extract_fd);

		if (!context.meta->fontName) {
			context.meta->fontName = sdsget_cff_sid(391, cffFile->name);
		}

		// We have FDArray
		if (cffFile->font_dict.count) {
			context.meta->fdArrayCount = cffFile->font_dict.count;
			NEW_N(context.meta->fdArray, context.meta->fdArrayCount);
			for (uint16_t j = 0; j < context.meta->fdArrayCount; j++) {
				context.meta->fdArray[j] = caryll_new_CFF();
				context.fdArrayIndex = j;
				cff_dict_callback(cffFile->font_dict.data + cffFile->font_dict.offset[j] - 1,
				                  cffFile->font_dict.offset[j + 1] - cffFile->font_dict.offset[j],
				                  &context, callback_extract_fd);
			}
		}
		ret.meta = context.meta;

		// Extract data of outlines
		context.seed = 0x1234567887654321;
		if (context.meta->privateDict) {
			context.seed =
			    (uint64_t)context.meta->privateDict->initialRandomSeed ^ 0x1234567887654321;
		}
		table_glyf *glyphs;
		NEW(glyphs);
		context.glyphs = glyphs;
		glyphs->numberGlyphs = cffFile->char_strings.count;
		NEW_N(glyphs->glyphs, glyphs->numberGlyphs);
		for (uint16_t j = 0; j < glyphs->numberGlyphs; j++) { buildOutline(j, &context); }

		// Name glyphs according charset
		nameGlyphsAccordingToCFF(&context);

		ret.glyphs = context.glyphs;
		CFF_close(cffFile);
	}
	return ret;
}

void pdDeltaToJson(json_value *target, const char *field, uint16_t count, float *values) {
	if (!count || !values) return;
	json_value *a = json_array_new(count);
	for (uint16_t j = 0; j < count; j++) { json_array_push(a, json_double_new(values[j])); }
	json_object_push(target, field, a);
}

static json_value *pdToJson(cff_private *pd) {
	json_value *_pd = json_object_new(24);
	pdDeltaToJson(_pd, "blueValues", pd->blueValuesCount, pd->blueValues);
	pdDeltaToJson(_pd, "otherBlues", pd->otherBluesCount, pd->otherBlues);
	pdDeltaToJson(_pd, "familyBlues", pd->familyBluesCount, pd->familyBlues);
	pdDeltaToJson(_pd, "familyOtherBlues", pd->familyOtherBluesCount, pd->familyOtherBlues);
	pdDeltaToJson(_pd, "stemSnapH", pd->stemSnapHCount, pd->stemSnapH);
	pdDeltaToJson(_pd, "stemSnapV", pd->stemSnapVCount, pd->stemSnapV);
	if (pd->blueScale != DEFAULT_BLUE_SCALE)
		json_object_push(_pd, "blueScale", json_double_new(pd->blueScale));
	if (pd->blueShift != DEFAULT_BLUE_SHIFT)
		json_object_push(_pd, "blueShift", json_double_new(pd->blueShift));
	if (pd->blueFuzz != DEFAULT_BLUE_FUZZ)
		json_object_push(_pd, "blueFuzz", json_double_new(pd->blueFuzz));
	if (pd->stdHW) json_object_push(_pd, "stdHW", json_double_new(pd->stdHW));
	if (pd->stdVW) json_object_push(_pd, "stdVW", json_double_new(pd->stdVW));
	if (pd->forceBold) json_object_push(_pd, "forceBold", json_boolean_new(pd->forceBold));
	if (pd->languageGroup)
		json_object_push(_pd, "languageGroup", json_double_new(pd->languageGroup));
	if (pd->expansionFactor != DEFAULT_EXPANSION_FACTOR)
		json_object_push(_pd, "expansionFactor", json_double_new(pd->expansionFactor));
	if (pd->initialRandomSeed)
		json_object_push(_pd, "initialRandomSeed", json_double_new(pd->initialRandomSeed));
	if (pd->defaultWidthX)
		json_object_push(_pd, "defaultWidthX", json_double_new(pd->defaultWidthX));
	if (pd->nominalWidthX)
		json_object_push(_pd, "nominalWidthX", json_double_new(pd->nominalWidthX));
	return _pd;
}
static json_value *fdToJson(table_CFF *table) {
	json_value *_CFF_ = json_object_new(24);

	if (table->isCID) json_object_push(_CFF_, "isCID", json_boolean_new(table->isCID));

	if (table->version) json_object_push(_CFF_, "version", json_from_sds(table->version));
	if (table->notice) json_object_push(_CFF_, "notice", json_from_sds(table->notice));
	if (table->copyright) json_object_push(_CFF_, "copyright", json_from_sds(table->copyright));
	if (table->fontName) json_object_push(_CFF_, "fontName", json_from_sds(table->fontName));
	if (table->fullName) json_object_push(_CFF_, "fullName", json_from_sds(table->fullName));
	if (table->familyName) json_object_push(_CFF_, "familyName", json_from_sds(table->familyName));
	if (table->weight) json_object_push(_CFF_, "weight", json_from_sds(table->weight));

	if (table->isFixedPitch)
		json_object_push(_CFF_, "isFixedPitch", json_boolean_new(table->isFixedPitch));
	if (table->italicAngle)
		json_object_push(_CFF_, "italicAngle", json_double_new(table->italicAngle));
	if (table->underlinePosition != -100)
		json_object_push(_CFF_, "underlinePosition", json_double_new(table->underlinePosition));
	if (table->underlineThickness != 50)
		json_object_push(_CFF_, "underlineThickness", json_double_new(table->underlineThickness));
	if (table->strokeWidth)
		json_object_push(_CFF_, "strokeWidth", json_double_new(table->strokeWidth));
	if (table->fontBBoxLeft)
		json_object_push(_CFF_, "fontBBoxLeft", json_double_new(table->fontBBoxLeft));
	if (table->fontBBoxBottom)
		json_object_push(_CFF_, "fontBBoxBottom", json_double_new(table->fontBBoxBottom));
	if (table->fontBBoxRight)
		json_object_push(_CFF_, "fontBBoxRight", json_double_new(table->fontBBoxRight));
	if (table->fontBBoxTop)
		json_object_push(_CFF_, "fontBBoxTop", json_double_new(table->fontBBoxTop));

	if (table->fontMatrix) {
		json_value *_fontMatrix = json_object_new(6);
		json_object_push(_fontMatrix, "a", json_double_new(table->fontMatrix->a));
		json_object_push(_fontMatrix, "b", json_double_new(table->fontMatrix->b));
		json_object_push(_fontMatrix, "c", json_double_new(table->fontMatrix->c));
		json_object_push(_fontMatrix, "d", json_double_new(table->fontMatrix->d));
		json_object_push(_fontMatrix, "x", json_double_new(table->fontMatrix->x));
		json_object_push(_fontMatrix, "y", json_double_new(table->fontMatrix->y));
		json_object_push(_CFF_, "fontMatrix", _fontMatrix);
	}
	if (table->privateDict) { json_object_push(_CFF_, "privates", pdToJson(table->privateDict)); }

	if (table->cidRegistry && table->cidOrdering) {
		json_object_push(_CFF_, "cidRegistry", json_from_sds(table->cidRegistry));
		json_object_push(_CFF_, "cidOrdering", json_from_sds(table->cidOrdering));
		json_object_push(_CFF_, "cidSupplement", json_double_new(table->cidSupplement));
	}
	if (table->fdArray) {
		json_value *_fdArray = json_array_new(table->fdArrayCount);
		for (uint16_t j = 0; j < table->fdArrayCount; j++) {
			json_array_push(_fdArray, fdToJson(table->fdArray[j]));
		}
		json_object_push(_CFF_, "fdArray", _fdArray);
	}
	return _CFF_;
}

void caryll_CFF_to_json(table_CFF *table, json_value *root, caryll_dump_options *dumpopts) {
	if (!table) return;
	json_object_push(root, "CFF_", fdToJson(table));
}

static void pdDeltaFromJson(json_value *dump, uint16_t *count, float **array) {
	if (!dump || dump->type != json_array) return;
	*count = dump->u.array.length;
	NEW_N(*array, *count);
	for (uint16_t j = 0; j < *count; j++) { (*array)[j] = json_numof(dump->u.array.values[j]); }
}
static cff_private *pdFromJson(json_value *dump) {
	if (!dump || dump->type != json_object) return NULL;
	cff_private *pd = caryll_new_CFF_private();
	pdDeltaFromJson(json_obj_get(dump, "blueValues"), &(pd->blueValuesCount), &(pd->blueValues));
	pdDeltaFromJson(json_obj_get(dump, "otherBlues"), &(pd->otherBluesCount), &(pd->otherBlues));
	pdDeltaFromJson(json_obj_get(dump, "familyBlues"), &(pd->familyBluesCount), &(pd->familyBlues));
	pdDeltaFromJson(json_obj_get(dump, "familyOtherBlues"), &(pd->familyOtherBluesCount),
	                &(pd->familyOtherBlues));
	pdDeltaFromJson(json_obj_get(dump, "stemSnapH"), &(pd->stemSnapHCount), &(pd->stemSnapH));
	pdDeltaFromJson(json_obj_get(dump, "stemSnapV"), &(pd->stemSnapVCount), &(pd->stemSnapV));

	pd->blueScale = json_obj_getnum_fallback(dump, "blueScale", DEFAULT_BLUE_SCALE);
	pd->blueShift = json_obj_getnum_fallback(dump, "blueShift", DEFAULT_BLUE_SHIFT);
	pd->blueFuzz = json_obj_getnum_fallback(dump, "blueFuzz", DEFAULT_BLUE_FUZZ);
	pd->stdHW = json_obj_getnum(dump, "stdHW");
	pd->stdVW = json_obj_getnum(dump, "stdVW");
	pd->forceBold = json_obj_getbool(dump, "forceBold");
	pd->languageGroup = json_obj_getnum(dump, "languageGroup");
	pd->expansionFactor =
	    json_obj_getnum_fallback(dump, "expansionFactor", DEFAULT_EXPANSION_FACTOR);
	pd->initialRandomSeed = json_obj_getnum(dump, "initialRandomSeed");
	// Not used -- they will be automatically calculated
	// pd->defaultWidthX = json_obj_getnum(dump, "defaultWidthX");
	// pd->nominalWidthX = json_obj_getnum(dump, "nominalWidthX");

	return pd;
}
static table_CFF *fdFromJson(json_value *dump) {
	table_CFF *table = caryll_new_CFF();
	if (!dump || dump->type != json_object) return table;
	// Names
	table->version = json_obj_getsds(dump, "version");
	table->notice = json_obj_getsds(dump, "notice");
	table->copyright = json_obj_getsds(dump, "copyright");
	table->fontName = json_obj_getsds(dump, "fontName");
	table->fullName = json_obj_getsds(dump, "fullName");
	table->familyName = json_obj_getsds(dump, "familyName");
	table->weight = json_obj_getsds(dump, "weight");

	// Metrics
	table->isFixedPitch = json_obj_getbool(dump, "isFixedPitch");
	table->italicAngle = json_obj_getnum(dump, "italicAngle");
	table->underlinePosition = json_obj_getnum_fallback(dump, "underlinePosition", -100.0);
	table->underlineThickness = json_obj_getnum_fallback(dump, "underlineThickness", 50.0);
	table->strokeWidth = json_obj_getnum(dump, "strokeWidth");
	table->fontBBoxLeft = json_obj_getnum(dump, "fontBBoxLeft");
	table->fontBBoxBottom = json_obj_getnum(dump, "fontBBoxBottom");
	table->fontBBoxRight = json_obj_getnum(dump, "fontBBoxRight");
	table->fontBBoxTop = json_obj_getnum(dump, "fontBBoxTop");

	// fontMatrix
	json_value *fmatdump = json_obj_get_type(dump, "fontMatrix", json_object);
	if (fmatdump) {
		NEW(table->fontMatrix);
		table->fontMatrix->a = json_obj_getnum(fmatdump, "a");
		table->fontMatrix->b = json_obj_getnum(fmatdump, "b");
		table->fontMatrix->c = json_obj_getnum(fmatdump, "c");
		table->fontMatrix->d = json_obj_getnum(fmatdump, "d");
		table->fontMatrix->x = json_obj_getnum(fmatdump, "x");
		table->fontMatrix->y = json_obj_getnum(fmatdump, "y");
	}

	// privates
	table->privateDict = pdFromJson(json_obj_get_type(dump, "privates", json_object));

	// CID
	table->cidRegistry = json_obj_getsds(dump, "cidRegistry");
	table->cidOrdering = json_obj_getsds(dump, "cidOrdering");
	table->cidSupplement = json_obj_getint(dump, "cidSupplement");
	table->UIDBase = json_obj_getint(dump, "UIDBase");
	table->cidCount = json_obj_getint(dump, "cidCount");
	table->cidFontVersion = json_obj_getnum(dump, "cidFontVersion");
	table->cidFontRevision = json_obj_getnum(dump, "cidFontRevision");

	// fdArray
	json_value *fdarraydump = json_obj_get_type(dump, "fdArray", json_array);
	if (fdarraydump && table->cidRegistry && table->cidOrdering) {
		table->isCID = true;
		table->fdArrayCount = fdarraydump->u.array.length;
		NEW_N(table->fdArray, table->fdArrayCount);
		for (uint16_t j = 0; j < table->fdArrayCount; j++) {
			table->fdArray[j] = fdFromJson(fdarraydump->u.array.values[j]);
		}
	}
	if (!table->privateDict) table->privateDict = caryll_new_CFF_private();
	return table;
}
table_CFF *caryll_CFF_from_json(json_value *root, caryll_dump_options *dumpopts) {
	json_value *dump = json_obj_get_type(root, "CFF_", json_object);
	if (!dump)
		return NULL;
	else
		return fdFromJson(dump);
}

static void cff_delete_dict(CFF_Dict *dict) {
	if (!dict) return;
	for (uint32_t j = 0; j < dict->count; j++) { free(dict->ents[j].vals); }
	free(dict->ents);
	free(dict);
}

typedef enum {
	IL_ITEM_OPERAND,
	IL_ITEM_OPERATOR,
	IL_ITEM_SPECIAL,
	IL_ITEM_PHANTOM_OPERATOR,
	IL_ITEM_PHANTOM_OPERAND
} il_type;

typedef struct {
	il_type type;
	double d;
	int32_t i;
	uint16_t arity;
} charstring_instruction;

typedef struct {
	uint32_t length;
	uint32_t free;
	charstring_instruction *instr;
} charstring_il;

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

static void pushMarks(charstring_il *il, glyf_glyph *g, uint16_t points, uint16_t *jh,
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

static charstring_il *compile_glyph_to_il(glyf_glyph *g, uint16_t defaultWidth,
                                          uint16_t nominalWidth) {
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
	uint16_t pointsSofar = 0;
	uint16_t jh = 0;
	uint16_t jm = 0;
	if (hasmask) pushMarks(il, g, pointsSofar, &jh, &jm);
	for (uint16_t c = 0; c < g->numberOfContours; c++) {
		glyf_contour *contour = &(g->contours[c]);
		uint16_t n = contour->pointsCount;
		if (n == 0) continue;
		il_push_operand(il, contour->points[0].x);
		il_push_operand(il, contour->points[0].y);
		il_push_op(il, op_rmoveto);
		pointsSofar++;
		if (hasmask) pushMarks(il, g, pointsSofar, &jh, &jm);
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
			if (hasmask) pushMarks(il, g, pointsSofar, &jh, &jm);
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

static void glyph_il_peephole_optimization(charstring_il *il) {
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

static caryll_buffer *il2blob(charstring_il *il) {
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

static caryll_buffer *compile_glyph(glyf_glyph *g, uint16_t defaultWidth, uint16_t nominalWidthX) {
	charstring_il *il = compile_glyph_to_il(g, defaultWidth, nominalWidthX);
	glyph_il_peephole_optimization(il);
	caryll_buffer *blob = il2blob(il);
	free(il->instr);
	free(il);
	return blob;
}

static caryll_buffer *compile_glyf_to_charstring(table_glyf *glyf, uint16_t defaultWidth,
                                                 uint16_t nominalWidthX) {
	if (glyf->numberGlyphs == 0) return bufnew();
	CFF_INDEX *charstring = cff_index_init();
	charstring->count = glyf->numberGlyphs;
	charstring->offSize = 4;
	NEW_N(charstring->offset, charstring->count + 1);
	charstring->offset[0] = 1;
	charstring->data = NULL;

	size_t used = 0;
	size_t blank = 0;
	for (uint32_t j = 0; j < glyf->numberGlyphs; j++) {
		caryll_buffer *blob = compile_glyph(glyf->glyphs[j], defaultWidth, nominalWidthX);
		if (blank < blob->size) {
			used += blob->size;
			blank = (used >> 1) & 0xFFFFFF;
			charstring->data = realloc(charstring->data, sizeof(uint8_t) * (used + blank));
		} else {
			used += blob->size;
			blank -= blob->size;
		}
		charstring->offset[j + 1] = (uint32_t)(blob->size + charstring->offset[j]);
		memcpy(charstring->data + charstring->offset[j] - 1, blob->data, blob->size);
		buffree(blob);
	}
	caryll_buffer *final_blob = compile_index(*charstring);
	cff_index_fini(charstring);
	return final_blob;
}

// String table management
typedef struct {
	int sid;
	char *str;
	UT_hash_handle hh;
} cff_sid_entry;

static int sidof(cff_sid_entry **h, sds s) {
	cff_sid_entry *item = NULL;
	HASH_FIND_STR(*h, s, item);
	if (item) {
		return 391 + item->sid;
	} else {
		NEW(item);
		item->sid = HASH_COUNT(*h);
		item->str = sdsdup(s);
		HASH_ADD_STR(*h, str, item);
		return 391 + item->sid;
	}
}

static CFF_Dict_Entry *cffdict_givemeablank(CFF_Dict *dict) {
	dict->count++;
	if (dict->ents) {
		dict->ents = realloc(dict->ents, dict->count * sizeof(CFF_Dict_Entry));
	} else {
		dict->ents = calloc(dict->count, sizeof(CFF_Dict_Entry));
	}
	return &(dict->ents[dict->count - 1]);
}
static void cffdict_input(CFF_Dict *dict, uint32_t op, CFF_Value_Type t, uint16_t arity, ...) {
	CFF_Dict_Entry *last = cffdict_givemeablank(dict);
	last->op = op;
	last->cnt = arity;
	NEW_N(last->vals, arity);

	va_list ap;
	va_start(ap, arity);
	for (uint16_t j = 0; j < arity; j++) {
		if (t == CFF_DOUBLE) {
			double x = va_arg(ap, double);
			if (x == round(x)) {
				last->vals[j].t = CFF_INTEGER;
				last->vals[j].i = round(x);
			} else {
				last->vals[j].t = CFF_DOUBLE;
				last->vals[j].d = x;
			}
		} else {
			int x = va_arg(ap, int);
			last->vals[j].t = t;
			last->vals[j].i = x;
		}
	}
	va_end(ap);
}
static void cffdict_input_aray(CFF_Dict *dict, uint32_t op, CFF_Value_Type t, uint16_t arity,
                               float *arr) {
	if (!arity || !arr) return;
	CFF_Dict_Entry *last = cffdict_givemeablank(dict);
	last->op = op;
	last->cnt = arity;
	NEW_N(last->vals, arity);
	for (uint16_t j = 0; j < arity; j++) {
		double x = arr[j];
		if (t == CFF_DOUBLE) {
			if (x == round(x)) {
				last->vals[j].t = CFF_INTEGER;
				last->vals[j].i = round(x);
			} else {
				last->vals[j].t = CFF_DOUBLE;
				last->vals[j].d = x;
			}
		} else {
			last->vals[j].t = t;
			last->vals[j].i = round(x);
		}
	}
}

static CFF_Dict *cff_make_fd_dict(table_CFF *fd, cff_sid_entry **h) {
	CFF_Dict *dict;
	NEW_CLEAN(dict);

	// ROS
	if (fd->cidRegistry && fd->cidOrdering) {
		cffdict_input(dict, op_ROS, CFF_INTEGER, 3, sidof(h, fd->cidRegistry),
		              sidof(h, fd->cidOrdering), fd->cidSupplement);
	}

	// CFF Names
	if (fd->version) cffdict_input(dict, op_version, CFF_INTEGER, 1, sidof(h, fd->version));
	if (fd->notice) cffdict_input(dict, op_Notice, CFF_INTEGER, 1, sidof(h, fd->notice));
	if (fd->copyright) cffdict_input(dict, op_Copyright, CFF_INTEGER, 1, sidof(h, fd->copyright));
	if (fd->fullName) cffdict_input(dict, op_FullName, CFF_INTEGER, 1, sidof(h, fd->fullName));
	if (fd->familyName)
		cffdict_input(dict, op_FamilyName, CFF_INTEGER, 1, sidof(h, fd->familyName));
	if (fd->weight) cffdict_input(dict, op_Weight, CFF_INTEGER, 1, sidof(h, fd->weight));

	// CFF Metrics
	cffdict_input(dict, op_FontBBox, CFF_DOUBLE, 4, fd->fontBBoxLeft, fd->fontBBoxBottom,
	              fd->fontBBoxRight, fd->fontBBoxTop);
	cffdict_input(dict, op_isFixedPitch, CFF_INTEGER, 1, (int)fd->isFixedPitch);
	cffdict_input(dict, op_ItalicAngle, CFF_DOUBLE, 1, fd->italicAngle);
	cffdict_input(dict, op_UnderlinePosition, CFF_DOUBLE, 1, fd->underlinePosition);
	cffdict_input(dict, op_UnderlineThickness, CFF_DOUBLE, 1, fd->underlineThickness);
	cffdict_input(dict, op_StrokeWidth, CFF_DOUBLE, 1, fd->strokeWidth);
	if (fd->fontMatrix) {
		cffdict_input(dict, op_FontMatrix, CFF_DOUBLE, 6, fd->fontMatrix->a, fd->fontMatrix->b,
		              fd->fontMatrix->c, fd->fontMatrix->d, fd->fontMatrix->x, fd->fontMatrix->y);
	}

	// CID specific
	if (fd->fontName) cffdict_input(dict, op_FontName, CFF_INTEGER, 1, sidof(h, fd->fontName));
	if (fd->cidFontVersion)
		cffdict_input(dict, op_CIDFontVersion, CFF_DOUBLE, 1, fd->cidFontVersion);
	if (fd->cidFontRevision)
		cffdict_input(dict, op_CIDFontRevision, CFF_DOUBLE, 1, fd->cidFontRevision);
	if (fd->cidCount) cffdict_input(dict, op_CIDCount, CFF_INTEGER, 1, fd->cidCount);
	if (fd->UIDBase) cffdict_input(dict, op_UIDBase, CFF_INTEGER, 1, fd->UIDBase);
	return dict;
}

static CFF_Dict *cff_make_private_dict(cff_private *pd) {
	CFF_Dict *dict;
	NEW_CLEAN(dict);
	if (!pd) return dict;
	// DELTA arrays
	cffdict_input_aray(dict, op_BlueValues, CFF_DOUBLE, pd->blueValuesCount, pd->blueValues);
	cffdict_input_aray(dict, op_OtherBlues, CFF_DOUBLE, pd->otherBluesCount, pd->otherBlues);
	cffdict_input_aray(dict, op_FamilyBlues, CFF_DOUBLE, pd->familyBluesCount, pd->familyBlues);
	cffdict_input_aray(dict, op_FamilyOtherBlues, CFF_DOUBLE, pd->familyOtherBluesCount,
	                   pd->familyOtherBlues);
	cffdict_input_aray(dict, op_StemSnapH, CFF_DOUBLE, pd->stemSnapHCount, pd->stemSnapH);
	cffdict_input_aray(dict, op_StemSnapV, CFF_DOUBLE, pd->stemSnapVCount, pd->stemSnapV);

	// Private scalars
	cffdict_input(dict, op_BlueScale, CFF_DOUBLE, 1, pd->blueScale);
	cffdict_input(dict, op_BlueShift, CFF_DOUBLE, 1, pd->blueShift);
	cffdict_input(dict, op_BlueFuzz, CFF_DOUBLE, 1, pd->blueFuzz);
	cffdict_input(dict, op_StdHW, CFF_DOUBLE, 1, pd->stdHW);
	cffdict_input(dict, op_StdVW, CFF_DOUBLE, 1, pd->stdVW);
	cffdict_input(dict, op_ForceBold, CFF_INTEGER, 1, (int)pd->forceBold);
	cffdict_input(dict, op_LanguageGroup, CFF_INTEGER, 1, pd->languageGroup);
	cffdict_input(dict, op_ExpansionFactor, CFF_DOUBLE, 1, pd->expansionFactor);
	cffdict_input(dict, op_initialRandomSeed, CFF_DOUBLE, 1, pd->initialRandomSeed);

	// op_nominalWidthX are currently not used
	cffdict_input(dict, op_defaultWidthX, CFF_DOUBLE, 1, pd->defaultWidthX);
	cffdict_input(dict, op_nominalWidthX, CFF_DOUBLE, 1, pd->nominalWidthX);
	return dict;
}

static caryll_buffer *cff_compileDict(CFF_Dict *dict) {
	caryll_buffer *blob = bufnew();
	for (uint32_t i = 0; i < dict->count; i++) {
		for (uint32_t j = 0; j < dict->ents[i].cnt; j++) {
			caryll_buffer *blob_val;
			if (dict->ents[i].vals[j].t == CFF_INTEGER) {
				blob_val = encode_cff_number(dict->ents[i].vals[j].i);
			} else if (dict->ents[i].vals[j].t == CFF_DOUBLE) {
				blob_val = encode_cff_real(dict->ents[i].vals[j].d);
			} else {
				blob_val = encode_cff_number(0);
			}
			bufwrite_bufdel(blob, blob_val);
		}
		bufwrite_bufdel(blob, encode_cff_operator(dict->ents[i].op));
	}
	return blob;
}

static int by_sid(cff_sid_entry *a, cff_sid_entry *b) { return a->sid - b->sid; }
static caryll_buffer *cffstrings_to_indexblob(cff_sid_entry **h) {
	HASH_SORT(*h, by_sid);
	CFF_INDEX *strings = cff_index_init();
	strings->count = HASH_COUNT(*h);
	strings->offSize = 4;
	NEW_N(strings->offset, strings->count + 1);
	strings->offset[0] = 1;
	strings->data = NULL;

	cff_sid_entry *item, *tmp;
	uint32_t j = 0;

	size_t used = 0;
	size_t blanks = 0;

	HASH_ITER(hh, *h, item, tmp) {
		strings->offset[j + 1] = (uint32_t)(sdslen(item->str) + strings->offset[j]);
		if (blanks < sdslen(item->str)) {
			used += sdslen(item->str);
			blanks = (used >> 1) & 0xFFFFFF;
			strings->data = realloc(strings->data, sizeof(uint8_t) * used + blanks);
		} else {
			blanks -= sdslen(item->str);
			used += sdslen(item->str);
		}
		memcpy(strings->data + strings->offset[j] - 1, item->str, sdslen(item->str));
		sdsfree(item->str);
		HASH_DEL(*h, item);
		free(item);
		j++;
	}
	caryll_buffer *final_blob = compile_index(*strings);
	cff_index_fini(strings);
	final_blob->cursor = final_blob->size;
	return final_blob;
}

static caryll_buffer *cff_compile_nameindex(table_CFF *cff) {
	CFF_INDEX *nameIndex = cff_index_init();
	nameIndex->count = 1;
	nameIndex->offSize = 4;
	NEW_N(nameIndex->offset, 2);
	if (!cff->fontName) { cff->fontName = sdsnew("Caryll-CFF-FONT"); }
	nameIndex->offset[0] = 1;
	nameIndex->offset[1] = (uint32_t)(sdslen(cff->fontName) + 1);
	NEW_N(nameIndex->data, 1 + sdslen(cff->fontName));
	memcpy(nameIndex->data, cff->fontName, sdslen(cff->fontName));
	// CFF Name INDEX
	caryll_buffer *buf = compile_index(*nameIndex);
	cff_index_fini(nameIndex);
	if (cff->fontName) {
		sdsfree(cff->fontName);
		cff->fontName = NULL;
	}
	return buf;
}

CFF_FDSelect *figureOutFDSelect(table_CFF *cff, table_glyf *glyf) {
	// We choose format 3 here
	uint32_t ranges = 1;
	uint8_t current = 0;
	CFF_FDSelect *fds;
	NEW_CLEAN(fds);
	fds->t = CFF_FDSELECT_UNSPECED;
	if (!glyf->numberGlyphs) return fds;
	uint8_t fdi0 = glyf->glyphs[0]->fdSelectIndex;
	if (fdi0 > cff->fdArrayCount) fdi0 = 0;
	current = fdi0;
	for (uint16_t j = 1; j < glyf->numberGlyphs; j++) {
		uint8_t fdi = glyf->glyphs[j]->fdSelectIndex;
		if (fdi > cff->fdArrayCount) fdi = 0;
		if (fdi != current) {
			current = fdi;
			ranges++;
		}
	}
	NEW_N(fds->f3.range3, ranges);
	fds->f3.range3[0].first = 0;
	fds->f3.range3[0].fd = current = fdi0;
	for (uint16_t j = 1; j < glyf->numberGlyphs; j++) {
		uint8_t fdi = glyf->glyphs[j]->fdSelectIndex;
		if (fdi > cff->fdArrayCount) fdi = 0;
		if (glyf->glyphs[j]->fdSelectIndex != current) {
			current = fdi;
			fds->s++;
			fds->f3.range3[fds->s].first = j;
			fds->f3.range3[fds->s].fd = current;
		}
	}
	fds->t = CFF_FDSELECT_FORMAT3;
	fds->s = ranges;
	fds->f3.format = 3;
	fds->f3.nranges = ranges;
	fds->f3.sentinel = glyf->numberGlyphs;
	return fds;
}

CFF_INDEX *cff_compile_fdarray(uint16_t fdArrayCount, table_CFF **fdArray,
                               cff_sid_entry **stringHash) {
	CFF_INDEX *newfd = cff_index_init();

	for (uint32_t i = 0; i < fdArrayCount; i++) {
		CFF_Dict *fd = cff_make_fd_dict(fdArray[i], stringHash);
		caryll_buffer *blob = cff_compileDict(fd);
		bufwrite_bufdel(blob, compile_offset(0xEEEEEEEE));
		bufwrite_bufdel(blob, compile_offset(0xFFFFFFFF));
		bufwrite_bufdel(blob, encode_cff_operator(op_Private));
		cff_delete_dict(fd);
		newfd->count += 1;
		newfd->offset = realloc(newfd->offset, (newfd->count + 1) * sizeof(uint32_t));
		if (newfd->count == 1) newfd->offset[0] = 1;
		newfd->offset[newfd->count] = (uint32_t)(blob->size + newfd->offset[newfd->count - 1]);
		newfd->data = realloc(newfd->data, newfd->offset[newfd->count] - 1);
		memcpy(newfd->data + newfd->offset[newfd->count - 1] - 1, blob->data, blob->size);
		buffree(blob);
	}

	newfd->offSize = 4;
	return newfd;
}

static caryll_buffer *writeCFF_CIDKeyed(table_CFF *cff, table_glyf *glyf) {
	caryll_buffer *blob = bufnew();
	// The Strings hashtable
	cff_sid_entry *stringHash = NULL;

	// CFF Header
	caryll_buffer *h = compile_header();
	caryll_buffer *n = cff_compile_nameindex(cff);

	// cff top DICT
	CFF_Dict *top = cff_make_fd_dict(cff, &stringHash);
	caryll_buffer *t = cff_compileDict(top);
	cff_delete_dict(top);
	// cff top PRIVATEs
	CFF_Dict *top_pd = cff_make_private_dict(cff->privateDict);
	caryll_buffer *p = cff_compileDict(top_pd);
	cff_delete_dict(top_pd);

	// FDSelect
	caryll_buffer *e;
	if (cff->isCID) {
		CFF_FDSelect *fds = figureOutFDSelect(cff, glyf);
		e = compile_fdselect(*fds);
		if (fds->t == CFF_FDSELECT_FORMAT3) FREE(fds->f3.range3);
		free(fds);
	} else {
		NEW_CLEAN(e);
	}

	// FDArray
	CFF_INDEX *fdArrayIndex = NULL;
	caryll_buffer *r;
	if (cff->isCID) {
		fdArrayIndex = cff_compile_fdarray(cff->fdArrayCount, cff->fdArray, &stringHash);
		r = compile_index(*fdArrayIndex);
	} else {
		NEW_CLEAN(r);
	}

	// cff charset : Allocate string index terms for glyph names
	CFF_Charset *charset;
	NEW_CLEAN(charset);
	if (glyf->numberGlyphs > 1) { // At least two glyphs
		charset->t = CFF_CHARSET_FORMAT2;
		charset->s = 1; // One segment only
		charset->f2.format = 2;
		NEW(charset->f2.range2);
		if (cff->isCID) {
			charset->f2.range2[0].first = 1;
			charset->f2.range2[0].nleft = glyf->numberGlyphs - 2;
		} else {
			for (uint16_t j = 1; j < glyf->numberGlyphs; j++) {
				sidof(&stringHash, glyf->glyphs[j]->name);
			}
			charset->f2.range2[0].first = sidof(&stringHash, glyf->glyphs[1]->name);
			charset->f2.range2[0].nleft = glyf->numberGlyphs - 2;
		}
	} else {
		charset->t = CFF_CHARSET_ISOADOBE;
	}
	caryll_buffer *c = compile_charset(*charset);
	if (charset->t == CFF_CHARSET_FORMAT2) { FREE(charset->f2.range2); }
	FREE(charset);

	// CFF String Index
	caryll_buffer *i = cffstrings_to_indexblob(&stringHash);

	// CFF Charstrings
	caryll_buffer *s = compile_glyf_to_charstring(glyf, cff->privateDict->defaultWidthX,
	                                              cff->privateDict->nominalWidthX);

	// Merge these data
	uint32_t delta = 0;
	uint32_t off = (uint32_t)(h->size + n->size + 11 + t->size);
	if (c->size != 0) delta += 6;
	if (e->size != 0) delta += 7;
	if (s->size != 0) delta += 6;
	if (p->size != 0) delta += 11;
	if (r->size != 0) delta += 7;
	bufwrite_bufdel(blob, h);
	bufwrite_bufdel(blob, n);
	int32_t delta_size = (uint32_t)(t->size + delta + 1);
	bufwrite_bufdel(blob, bufninit(11, 0, 1,   // one item
	                               4,          // four bytes per offset
	                               0, 0, 0, 1, // offset 1
	                               (delta_size >> 24) & 0xff, (delta_size >> 16) & 0xff,
	                               (delta_size >> 8) & 0xff,
	                               delta_size & 0xff) // offset 2
	                );
	bufwrite_bufdel(blob, t);

	// Compile offsets
	off += delta + i->size + 3;
	if (c->size != 0) {
		bufwrite_bufdel(blob, compile_offset(off));
		bufwrite_bufdel(blob, encode_cff_operator(op_charset));
		off += c->size;
	}
	if (e->size != 0) {
		bufwrite_bufdel(blob, compile_offset(off));
		bufwrite_bufdel(blob, encode_cff_operator(op_FDSelect));
		off += e->size;
	}
	if (s->size != 0) {
		bufwrite_bufdel(blob, compile_offset(off));
		bufwrite_bufdel(blob, encode_cff_operator(op_CharStrings));
		off += s->size;
	}
	if (p->size != 0) {
		bufwrite_bufdel(blob, compile_offset((uint32_t)(p->size)));
		bufwrite_bufdel(blob, compile_offset(off));
		bufwrite_bufdel(blob, encode_cff_operator(op_Private));
		off += p->size;
	}
	if (r->size != 0) {
		bufwrite_bufdel(blob, compile_offset(off));
		bufwrite_bufdel(blob, encode_cff_operator(op_FDArray));
		off += r->size;
	}

	bufwrite_bufdel(blob, i);
	bufwrite_bufdel(blob, bufninit(3, 0, 0, 0)); // gsubr
	bufwrite_bufdel(blob, c);
	bufwrite_bufdel(blob, e);
	bufwrite_bufdel(blob, s);
	bufwrite_bufdel(blob, p);
	if (cff->isCID) {
		uint32_t fdArrayPrivatesStartOffset = off;
		caryll_buffer **fdArrayPrivates;
		NEW_N(fdArrayPrivates, cff->fdArrayCount);
		for (uint16_t j = 0; j < cff->fdArrayCount; j++) {
			CFF_Dict *pd = cff_make_private_dict(cff->fdArray[j]->privateDict);
			caryll_buffer *p = cff_compileDict(pd);
			cff_delete_dict(pd);
			fdArrayPrivates[j] = p;
			uint8_t *privateLengthPtr = &(fdArrayIndex->data[fdArrayIndex->offset[j + 1] - 11]);
			privateLengthPtr[0] = (p->size >> 24) & 0xFF;
			privateLengthPtr[1] = (p->size >> 16) & 0xFF;
			privateLengthPtr[2] = (p->size >> 8) & 0xFF;
			privateLengthPtr[3] = (p->size >> 0) & 0xFF;
			uint8_t *privateOffsetPtr = &(fdArrayIndex->data[fdArrayIndex->offset[j + 1] - 6]);
			privateOffsetPtr[0] = (fdArrayPrivatesStartOffset >> 24) & 0xFF;
			privateOffsetPtr[1] = (fdArrayPrivatesStartOffset >> 16) & 0xFF;
			privateOffsetPtr[2] = (fdArrayPrivatesStartOffset >> 8) & 0xFF;
			privateOffsetPtr[3] = (fdArrayPrivatesStartOffset >> 0) & 0xFF;
			fdArrayPrivatesStartOffset += p->size;
		}
		buffree(r);
		r = compile_index(*fdArrayIndex);
		cff_index_fini(fdArrayIndex);
		bufwrite_bufdel(blob, r);
		for (uint16_t j = 0; j < cff->fdArrayCount; j++) {
			bufwrite_bufdel(blob, fdArrayPrivates[j]);
		}
		free(fdArrayPrivates);
	} else {
		bufwrite_bufdel(blob, r);
	}

	return blob;
}

caryll_buffer *caryll_write_CFF(caryll_cff_parse_result cffAndGlyf) {
	return writeCFF_CIDKeyed(cffAndGlyf.meta, cffAndGlyf.glyphs);
}
