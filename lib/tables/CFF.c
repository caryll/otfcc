#include "CFF.h"

const double DEFAULT_BLUE_SCALE = 0.039625;
const double DEFAULT_BLUE_SHIFT = 7;
const double DEFAULT_BLUE_FUZZ = 1;
const double DEFAULT_EXPANSION_FACTOR = 0.06;

static cff_PrivateDict *table_new_cff_private() {
	cff_PrivateDict *pd = NULL;
	NEW_CLEAN(pd);
	pd->blueFuzz = DEFAULT_BLUE_FUZZ;
	pd->blueScale = DEFAULT_BLUE_SCALE;
	pd->blueShift = DEFAULT_BLUE_SHIFT;
	pd->expansionFactor = DEFAULT_EXPANSION_FACTOR;
	return pd;
}
table_CFF *table_new_CFF() {
	table_CFF *table = NULL;
	NEW_CLEAN(table);
	table->underlinePosition = -100;
	table->underlineThickness = 50;
	return table;
}

static void caryll_delete_privatedict(cff_PrivateDict *priv) {
	if (!priv) return;
	FREE(priv->blueValues);
	FREE(priv->otherBlues);
	FREE(priv->familyBlues);

	FREE(priv->familyOtherBlues);
	FREE(priv->stemSnapH);
	FREE(priv->stemSnapV);
	FREE(priv);
}
void table_delete_CFF(table_CFF *table) {
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
		for (uint16_t j = 0; j < table->fdArrayCount; j++) {
			table_delete_CFF(table->fdArray[j]);
		}
		free(table->fdArray);
	}
	FREE(table);
}

typedef struct {
	int32_t fdArrayIndex;
	table_CFF *meta;
	table_glyf *glyphs;
	cff_File *cffFile;
	uint64_t seed;
} cff_extract_context;
static void callback_extract_private(uint32_t op, uint8_t top, cff_Value *stack, void *_context) {
	cff_extract_context *context = (cff_extract_context *)_context;
	table_CFF *meta = context->meta;
	if (context->fdArrayIndex >= 0 && context->fdArrayIndex < meta->fdArrayCount) {
		meta = meta->fdArray[context->fdArrayIndex];
	}
	cff_PrivateDict *pd = meta->privateDict;
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

static void callback_extract_fd(uint32_t op, uint8_t top, cff_Value *stack, void *_context) {
	cff_extract_context *context = (cff_extract_context *)_context;
	cff_File *file = context->cffFile;
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
				meta->privateDict = table_new_cff_private();
				cff_extract_DictByCallback(file->raw_data + privateOffset, privateLength, context,
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
	glyf_Glyph *g;
	uint16_t jContour;
	uint16_t jPoint;
	double defaultWidthX;
	double nominalWidthX;
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
static void callback_countpoint_lineto(void *_context, double x1, double y1) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (context->jContour) {
		context->g->contours[context->jContour - 1].pointsCount += 1;
		context->jPoint += 1;
	}
}
static void callback_countpoint_curveto(void *_context, double x1, double y1, double x2, double y2, double x3,
                                        double y3) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (context->jContour) {
		context->g->contours[context->jContour - 1].pointsCount += 3;
		context->jPoint += 3;
	}
}
static void callback_countpoint_sethint(void *_context, bool isVertical, double position, double width) {
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

static void callback_draw_setwidth(void *_context, double width) {
	outline_builder_context *context = (outline_builder_context *)_context;
	context->g->advanceWidth = width + context->nominalWidthX;
}
static void callback_draw_next_contour(void *_context) {
	outline_builder_context *context = (outline_builder_context *)_context;
	context->jContour += 1;
	context->jPoint = 0;
}
static void callback_draw_lineto(void *_context, double x1, double y1) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (context->jContour) {
		context->g->contours[context->jContour - 1].points[context->jPoint].onCurve = true;
		context->g->contours[context->jContour - 1].points[context->jPoint].x = x1;
		context->g->contours[context->jContour - 1].points[context->jPoint].y = y1;
		context->jPoint += 1;
		context->pointsDefined += 1;
	}
}
static void callback_draw_curveto(void *_context, double x1, double y1, double x2, double y2, double x3, double y3) {
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
static void callback_draw_sethint(void *_context, bool isVertical, double position, double width) {
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
	glyf_PostscriptHintMask *maskList = isContourMask ? context->g->contourMasks : context->g->hintMasks;
	glyf_PostscriptHintMask *mask;
	bool duplicateMask = false;

	if (maskIndex > 0) {
		glyf_PostscriptHintMask *lastMask = &(maskList)[maskIndex - 1];
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
		mask->maskV[j] = j < context->g->numberOfStemV ? maskArray[j + context->g->numberOfStemH] : 0;
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

static void buildOutline(uint16_t i, cff_extract_context *context) {
	cff_File *f = context->cffFile;
	glyf_Glyph *g = table_new_glyf_glyph();
	context->glyphs->glyphs[i] = g;
	uint64_t seed = context->seed;

	cff_Index localSubrs;
	cff_Stack stack;

	stack.index = 0;
	stack.stem = 0;

	outline_builder_context bc = {g, 0, 0, 0.0, 0.0, 0, 0, 0, 0, 0, 0};
	cff_IOutlineBuilder pass1 = {NULL, callback_count_contour, NULL, NULL, NULL, NULL, NULL};
	cff_IOutlineBuilder pass2 = {NULL,
	                             callback_countpoint_next_contour,
	                             callback_countpoint_lineto,
	                             callback_countpoint_curveto,
	                             callback_countpoint_sethint,
	                             callback_countpoint_setmask,
	                             NULL};
	cff_IOutlineBuilder pass3 = {callback_draw_setwidth, callback_draw_next_contour, callback_draw_lineto,
	                             callback_draw_curveto,  callback_draw_sethint,      callback_draw_setmask,
	                             callback_draw_getrand};

	uint8_t fd = 0;
	if (f->fdselect.t != cff_FDSELECT_UNSPECED)
		fd = cff_parseSubr(i, f->raw_data, f->font_dict, f->fdselect, &localSubrs);
	else
		fd = cff_parseSubr(i, f->raw_data, f->top_dict, f->fdselect, &localSubrs);

	g->fdSelect = handle_fromIndex(fd);
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
	cff_parseOutline(charStringPtr, charStringLength, f->global_subr, localSubrs, &stack, &bc, pass1);
	NEW_N(g->contours, g->numberOfContours);

	// PASS 2 : Count points
	stack.index = 0;
	stack.stem = 0;
	bc.randx = seed;
	cff_parseOutline(charStringPtr, charStringLength, f->global_subr, localSubrs, &stack, &bc, pass2);
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
	cff_parseOutline(charStringPtr, charStringLength, f->global_subr, localSubrs, &stack, &bc, pass3);
	g->numberOfContourMasks = bc.definedContourMasks;
	g->numberOfHintMasks = bc.definedHintMasks;

	// PASS 4 : Turn deltas into absolute coordinates
	double cx = 0;
	double cy = 0;
	for (uint16_t j = 0; j < g->numberOfContours; j++) {
		for (uint16_t k = 0; k < g->contours[j].pointsCount; k++) {
			cx += g->contours[j].points[k].x;
			cy += g->contours[j].points[k].y;

			g->contours[j].points[k].x = cx;
			g->contours[j].points[k].y = cy;
		}
	}
	cff_close_Index(localSubrs);
	context->seed = bc.randx;
}

static void nameGlyphsAccordingToCFF(cff_extract_context *context) {
	if (context->meta->isCID) return;
	cff_File *cffFile = context->cffFile;
	table_glyf *glyphs = context->glyphs;
	switch (cffFile->charsets.t) {
		case cff_CHARSET_FORMAT0: {
			for (uint16_t j = 0; j < cffFile->charsets.s; j++) {
				uint16_t sid = cffFile->charsets.f0.glyph[j];
				sds glyphname = sdsget_cff_sid(sid, cffFile->string);
				if (glyphname) { glyphs->glyphs[j + 1]->name = glyphname; }
			}
			break;
		}
		case cff_CHARSET_FORMAT1: {
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
		case cff_CHARSET_FORMAT2: {
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

table_CFFAndGlyf table_read_cff_and_glyf(caryll_Packet packet) {
	table_CFFAndGlyf ret;
	ret.meta = NULL;
	ret.glyphs = NULL;

	cff_extract_context context;
	context.fdArrayIndex = -1;
	context.meta = NULL;
	context.glyphs = NULL;
	context.cffFile = NULL;
	FOR_TABLE('CFF ', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		cff_File *cffFile = cff_openStream(data, length);
		context.cffFile = cffFile;
		context.meta = table_new_CFF();

		// Extract data in TOP DICT
		cff_extract_DictByCallback(cffFile->top_dict.data, cffFile->top_dict.offset[1] - cffFile->top_dict.offset[0],
		                           &context, callback_extract_fd);

		if (!context.meta->fontName) { context.meta->fontName = sdsget_cff_sid(391, cffFile->name); }

		// We have FDArray
		if (cffFile->font_dict.count) {
			context.meta->fdArrayCount = cffFile->font_dict.count;
			NEW_N(context.meta->fdArray, context.meta->fdArrayCount);
			for (uint16_t j = 0; j < context.meta->fdArrayCount; j++) {
				context.meta->fdArray[j] = table_new_CFF();
				context.fdArrayIndex = j;
				cff_extract_DictByCallback(cffFile->font_dict.data + cffFile->font_dict.offset[j] - 1,
				                           cffFile->font_dict.offset[j + 1] - cffFile->font_dict.offset[j], &context,
				                           callback_extract_fd);
				if (!context.meta->fdArray[j]->fontName) {
					context.meta->fdArray[j]->fontName = sdscatprintf(sdsempty(), "_Subfont%d", j);
				}
			}
		}
		ret.meta = context.meta;

		// Extract data of outlines
		context.seed = 0x1234567887654321;
		if (context.meta->privateDict) {
			context.seed = (uint64_t)context.meta->privateDict->initialRandomSeed ^ 0x1234567887654321;
		}
		table_glyf *glyphs;
		NEW(glyphs);
		context.glyphs = glyphs;
		glyphs->numberGlyphs = cffFile->char_strings.count;
		NEW_N(glyphs->glyphs, glyphs->numberGlyphs);
		for (uint16_t j = 0; j < glyphs->numberGlyphs; j++) {
			buildOutline(j, &context);
		}

		// Name glyphs according charset
		nameGlyphsAccordingToCFF(&context);

		ret.glyphs = context.glyphs;
		cff_close(cffFile);
	}
	return ret;
}

static void pdDeltaToJson(json_value *target, const char *field, uint16_t count, double *values) {
	if (!count || !values) return;
	json_value *a = json_array_new(count);
	for (uint16_t j = 0; j < count; j++) {
		json_array_push(a, json_double_new(values[j]));
	}
	json_object_push(target, field, a);
}

static json_value *pdToJson(cff_PrivateDict *pd) {
	json_value *_pd = json_object_new(24);
	pdDeltaToJson(_pd, "blueValues", pd->blueValuesCount, pd->blueValues);
	pdDeltaToJson(_pd, "otherBlues", pd->otherBluesCount, pd->otherBlues);
	pdDeltaToJson(_pd, "familyBlues", pd->familyBluesCount, pd->familyBlues);
	pdDeltaToJson(_pd, "familyOtherBlues", pd->familyOtherBluesCount, pd->familyOtherBlues);
	pdDeltaToJson(_pd, "stemSnapH", pd->stemSnapHCount, pd->stemSnapH);
	pdDeltaToJson(_pd, "stemSnapV", pd->stemSnapVCount, pd->stemSnapV);
	if (pd->blueScale != DEFAULT_BLUE_SCALE) json_object_push(_pd, "blueScale", json_double_new(pd->blueScale));
	if (pd->blueShift != DEFAULT_BLUE_SHIFT) json_object_push(_pd, "blueShift", json_double_new(pd->blueShift));
	if (pd->blueFuzz != DEFAULT_BLUE_FUZZ) json_object_push(_pd, "blueFuzz", json_double_new(pd->blueFuzz));
	if (pd->stdHW) json_object_push(_pd, "stdHW", json_double_new(pd->stdHW));
	if (pd->stdVW) json_object_push(_pd, "stdVW", json_double_new(pd->stdVW));
	if (pd->forceBold) json_object_push(_pd, "forceBold", json_boolean_new(pd->forceBold));
	if (pd->languageGroup) json_object_push(_pd, "languageGroup", json_double_new(pd->languageGroup));
	if (pd->expansionFactor != DEFAULT_EXPANSION_FACTOR)
		json_object_push(_pd, "expansionFactor", json_double_new(pd->expansionFactor));
	if (pd->initialRandomSeed) json_object_push(_pd, "initialRandomSeed", json_double_new(pd->initialRandomSeed));
	if (pd->defaultWidthX) json_object_push(_pd, "defaultWidthX", json_double_new(pd->defaultWidthX));
	if (pd->nominalWidthX) json_object_push(_pd, "nominalWidthX", json_double_new(pd->nominalWidthX));
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

	if (table->isFixedPitch) json_object_push(_CFF_, "isFixedPitch", json_boolean_new(table->isFixedPitch));
	if (table->italicAngle) json_object_push(_CFF_, "italicAngle", json_double_new(table->italicAngle));
	if (table->underlinePosition != -100)
		json_object_push(_CFF_, "underlinePosition", json_double_new(table->underlinePosition));
	if (table->underlineThickness != 50)
		json_object_push(_CFF_, "underlineThickness", json_double_new(table->underlineThickness));
	if (table->strokeWidth) json_object_push(_CFF_, "strokeWidth", json_double_new(table->strokeWidth));
	if (table->fontBBoxLeft) json_object_push(_CFF_, "fontBBoxLeft", json_double_new(table->fontBBoxLeft));
	if (table->fontBBoxBottom) json_object_push(_CFF_, "fontBBoxBottom", json_double_new(table->fontBBoxBottom));
	if (table->fontBBoxRight) json_object_push(_CFF_, "fontBBoxRight", json_double_new(table->fontBBoxRight));
	if (table->fontBBoxTop) json_object_push(_CFF_, "fontBBoxTop", json_double_new(table->fontBBoxTop));

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
		json_object_push(_CFF_, "cidSupplement", json_integer_new(table->cidSupplement));
	}
	if (table->fdArray) {
		json_value *_fdArray = json_object_new(table->fdArrayCount);
		for (uint16_t j = 0; j < table->fdArrayCount; j++) {
			sds name = table->fdArray[j]->fontName;
			table->fdArray[j]->fontName = NULL;
			json_object_push(_fdArray, name, fdToJson(table->fdArray[j]));
			table->fdArray[j]->fontName = name;
		}
		json_object_push(_CFF_, "fdArray", _fdArray);
	}
	return _CFF_;
}

void table_dump_cff(table_CFF *table, json_value *root, const caryll_Options *options) {
	if (!table) return;
	if (options->verbose) fprintf(stderr, "Dumping CFF.\n");

	json_object_push(root, "CFF_", fdToJson(table));
}

static void pdDeltaFromJson(json_value *dump, uint16_t *count, double **array) {
	if (!dump || dump->type != json_array) return;
	*count = dump->u.array.length;
	NEW_N(*array, *count);
	for (uint16_t j = 0; j < *count; j++) {
		(*array)[j] = json_numof(dump->u.array.values[j]);
	}
}
static cff_PrivateDict *pdFromJson(json_value *dump) {
	if (!dump || dump->type != json_object) return NULL;
	cff_PrivateDict *pd = table_new_cff_private();
	pdDeltaFromJson(json_obj_get(dump, "blueValues"), &(pd->blueValuesCount), &(pd->blueValues));
	pdDeltaFromJson(json_obj_get(dump, "otherBlues"), &(pd->otherBluesCount), &(pd->otherBlues));
	pdDeltaFromJson(json_obj_get(dump, "familyBlues"), &(pd->familyBluesCount), &(pd->familyBlues));
	pdDeltaFromJson(json_obj_get(dump, "familyOtherBlues"), &(pd->familyOtherBluesCount), &(pd->familyOtherBlues));
	pdDeltaFromJson(json_obj_get(dump, "stemSnapH"), &(pd->stemSnapHCount), &(pd->stemSnapH));
	pdDeltaFromJson(json_obj_get(dump, "stemSnapV"), &(pd->stemSnapVCount), &(pd->stemSnapV));

	pd->blueScale = json_obj_getnum_fallback(dump, "blueScale", DEFAULT_BLUE_SCALE);
	pd->blueShift = json_obj_getnum_fallback(dump, "blueShift", DEFAULT_BLUE_SHIFT);
	pd->blueFuzz = json_obj_getnum_fallback(dump, "blueFuzz", DEFAULT_BLUE_FUZZ);
	pd->stdHW = json_obj_getnum(dump, "stdHW");
	pd->stdVW = json_obj_getnum(dump, "stdVW");
	pd->forceBold = json_obj_getbool(dump, "forceBold");
	pd->languageGroup = json_obj_getnum(dump, "languageGroup");
	pd->expansionFactor = json_obj_getnum_fallback(dump, "expansionFactor", DEFAULT_EXPANSION_FACTOR);
	pd->initialRandomSeed = json_obj_getnum(dump, "initialRandomSeed");
	// Not used -- they will be automatically calculated
	// pd->defaultWidthX = json_obj_getnum(dump, "defaultWidthX");
	// pd->nominalWidthX = json_obj_getnum(dump, "nominalWidthX");

	return pd;
}
static table_CFF *fdFromJson(json_value *dump, const caryll_Options *options, bool topLevel) {
	table_CFF *table = table_new_CFF();
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
	// Need?
	/*
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
	*/

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
	json_value *fdarraydump = json_obj_get_type(dump, "fdArray", json_object);
	if (fdarraydump) {
		table->isCID = true;
		table->fdArrayCount = fdarraydump->u.object.length;
		NEW_N(table->fdArray, table->fdArrayCount);
		for (uint16_t j = 0; j < table->fdArrayCount; j++) {
			table->fdArray[j] = fdFromJson(fdarraydump->u.object.values[j].value, options, false);
			if (table->fdArray[j]->fontName) { sdsfree(table->fdArray[j]->fontName); }
			table->fdArray[j]->fontName =
			    sdsnewlen(fdarraydump->u.object.values[j].name, fdarraydump->u.object.values[j].name_length);
		}
	}
	if (!table->fontName) table->fontName = sdsnew("CARYLL_CFFFONT");
	if (!table->privateDict) table->privateDict = table_new_cff_private();
	// Deal with force-cid
	if (topLevel && options->force_cid && !table->fdArray) {
		table->fdArrayCount = 1;
		NEW_N(table->fdArray, table->fdArrayCount);
		table->fdArray[0] = table_new_CFF();
		table_CFF *fd0 = table->fdArray[0];
		fd0->privateDict = table->privateDict;
		table->privateDict = table_new_cff_private();
		fd0->fontName = sdscat(sdsdup(table->fontName), "-subfont0");
		table->isCID = true;
	}
	if (table->isCID && !table->cidRegistry) { table->cidRegistry = sdsnew("CARYLL"); }
	if (table->isCID && !table->cidOrdering) { table->cidOrdering = sdsnew("OTFCCAUTOCID"); }
	return table;
}
table_CFF *table_parse_cff(json_value *root, const caryll_Options *options) {
	json_value *dump = json_obj_get_type(root, "CFF_", json_object);
	if (!dump) {
		return NULL;
	} else {
		if (options->verbose) fprintf(stderr, "Parsing CFF.\n");
		return fdFromJson(dump, options, true);
	}
}

static caryll_buffer *compile_glyph(glyf_Glyph *g, uint16_t defaultWidth, uint16_t nominalWidthX,
                                    const caryll_Options *options, cff_SubrGraph *gr) {
	cff_CharstringIL *il = cff_compileGlyphToIL(g, defaultWidth, nominalWidthX);
	cff_optimizeIL(il, options);
	caryll_buffer *blob = cff_build_IL(il);
	cff_insertILToGraph(gr, il);
	free(il->instr);
	free(il);
	return blob;
}

typedef struct {
	table_glyf *glyf;
	uint16_t defaultWidth;
	uint16_t nominalWidthX;
	const caryll_Options *options;
	cff_SubrGraph *graph;
} cff_charstring_builder_context;
static caryll_buffer *callback_makeglyph(void *_context, uint32_t j) {
	cff_charstring_builder_context *context = (cff_charstring_builder_context *)_context;
	return compile_glyph(context->glyf->glyphs[j], context->defaultWidth, context->nominalWidthX, context->options,
	                     context->graph);
}
static caryll_buffer *cff_make_charstrings(cff_charstring_builder_context *context) {
	if (context->glyf->numberGlyphs == 0) return bufnew();
	cff_Index *charstring = cff_newIndexByCallback(context, context->glyf->numberGlyphs, callback_makeglyph);
	caryll_buffer *final_blob = cff_build_Index(*charstring);
	cff_delete_Index(charstring);
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

static cff_DictEntry *cffdict_givemeablank(cff_Dict *dict) {
	dict->count++;
	if (dict->ents) {
		dict->ents = realloc(dict->ents, dict->count * sizeof(cff_DictEntry));
	} else {
		dict->ents = calloc(dict->count, sizeof(cff_DictEntry));
	}
	return &(dict->ents[dict->count - 1]);
}
static void cffdict_input(cff_Dict *dict, uint32_t op, cff_Value_Type t, uint16_t arity, ...) {
	cff_DictEntry *last = cffdict_givemeablank(dict);
	last->op = op;
	last->cnt = arity;
	NEW_N(last->vals, arity);

	va_list ap;
	va_start(ap, arity);
	for (uint16_t j = 0; j < arity; j++) {
		if (t == cff_DOUBLE) {
			double x = va_arg(ap, double);
			if (x == round(x)) {
				last->vals[j].t = cff_INTEGER;
				last->vals[j].i = round(x);
			} else {
				last->vals[j].t = cff_DOUBLE;
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
static void cffdict_input_array(cff_Dict *dict, uint32_t op, cff_Value_Type t, uint16_t arity, double *arr) {
	if (!arity || !arr) return;
	cff_DictEntry *last = cffdict_givemeablank(dict);
	last->op = op;
	last->cnt = arity;
	NEW_N(last->vals, arity);
	for (uint16_t j = 0; j < arity; j++) {
		double x = arr[j];
		if (t == cff_DOUBLE) {
			if (x == round(x)) {
				last->vals[j].t = cff_INTEGER;
				last->vals[j].i = round(x);
			} else {
				last->vals[j].t = cff_DOUBLE;
				last->vals[j].d = x;
			}
		} else {
			last->vals[j].t = t;
			last->vals[j].i = round(x);
		}
	}
}

static cff_Dict *cff_make_fd_dict(table_CFF *fd, cff_sid_entry **h) {
	cff_Dict *dict;
	NEW_CLEAN(dict);
	// ROS
	if (fd->cidRegistry && fd->cidOrdering) {
		cffdict_input(dict, op_ROS, cff_INTEGER, 3, sidof(h, fd->cidRegistry), sidof(h, fd->cidOrdering),
		              fd->cidSupplement);
	}

	// CFF Names
	if (fd->version) cffdict_input(dict, op_version, cff_INTEGER, 1, sidof(h, fd->version));
	if (fd->notice) cffdict_input(dict, op_Notice, cff_INTEGER, 1, sidof(h, fd->notice));
	if (fd->copyright) cffdict_input(dict, op_Copyright, cff_INTEGER, 1, sidof(h, fd->copyright));
	if (fd->fullName) cffdict_input(dict, op_FullName, cff_INTEGER, 1, sidof(h, fd->fullName));
	if (fd->familyName) cffdict_input(dict, op_FamilyName, cff_INTEGER, 1, sidof(h, fd->familyName));
	if (fd->weight) cffdict_input(dict, op_Weight, cff_INTEGER, 1, sidof(h, fd->weight));

	// CFF Metrics
	cffdict_input(dict, op_FontBBox, cff_DOUBLE, 4, fd->fontBBoxLeft, fd->fontBBoxBottom, fd->fontBBoxRight,
	              fd->fontBBoxTop);
	cffdict_input(dict, op_isFixedPitch, cff_INTEGER, 1, (int)fd->isFixedPitch);
	cffdict_input(dict, op_ItalicAngle, cff_DOUBLE, 1, fd->italicAngle);
	cffdict_input(dict, op_UnderlinePosition, cff_DOUBLE, 1, fd->underlinePosition);
	cffdict_input(dict, op_UnderlineThickness, cff_DOUBLE, 1, fd->underlineThickness);
	cffdict_input(dict, op_StrokeWidth, cff_DOUBLE, 1, fd->strokeWidth);
	if (fd->fontMatrix) {
		cffdict_input(dict, op_FontMatrix, cff_DOUBLE, 6, fd->fontMatrix->a, fd->fontMatrix->b, fd->fontMatrix->c,
		              fd->fontMatrix->d, fd->fontMatrix->x, fd->fontMatrix->y);
	}

	// CID specific
	if (fd->fontName) cffdict_input(dict, op_FontName, cff_INTEGER, 1, sidof(h, fd->fontName));
	if (fd->cidFontVersion) cffdict_input(dict, op_CIDFontVersion, cff_DOUBLE, 1, fd->cidFontVersion);
	if (fd->cidFontRevision) cffdict_input(dict, op_CIDFontRevision, cff_DOUBLE, 1, fd->cidFontRevision);
	if (fd->cidCount) cffdict_input(dict, op_CIDCount, cff_INTEGER, 1, fd->cidCount);
	if (fd->UIDBase) cffdict_input(dict, op_UIDBase, cff_INTEGER, 1, fd->UIDBase);
	return dict;
}

static cff_Dict *cff_make_private_dict(cff_PrivateDict *pd) {
	cff_Dict *dict;
	NEW_CLEAN(dict);
	if (!pd) return dict;
	// DELTA arrays
	cffdict_input_array(dict, op_BlueValues, cff_DOUBLE, pd->blueValuesCount, pd->blueValues);
	cffdict_input_array(dict, op_OtherBlues, cff_DOUBLE, pd->otherBluesCount, pd->otherBlues);
	cffdict_input_array(dict, op_FamilyBlues, cff_DOUBLE, pd->familyBluesCount, pd->familyBlues);
	cffdict_input_array(dict, op_FamilyOtherBlues, cff_DOUBLE, pd->familyOtherBluesCount, pd->familyOtherBlues);
	cffdict_input_array(dict, op_StemSnapH, cff_DOUBLE, pd->stemSnapHCount, pd->stemSnapH);
	cffdict_input_array(dict, op_StemSnapV, cff_DOUBLE, pd->stemSnapVCount, pd->stemSnapV);

	// Private scalars
	cffdict_input(dict, op_BlueScale, cff_DOUBLE, 1, pd->blueScale);
	cffdict_input(dict, op_BlueShift, cff_DOUBLE, 1, pd->blueShift);
	cffdict_input(dict, op_BlueFuzz, cff_DOUBLE, 1, pd->blueFuzz);
	cffdict_input(dict, op_StdHW, cff_DOUBLE, 1, pd->stdHW);
	cffdict_input(dict, op_StdVW, cff_DOUBLE, 1, pd->stdVW);
	cffdict_input(dict, op_ForceBold, cff_INTEGER, 1, (int)pd->forceBold);
	cffdict_input(dict, op_LanguageGroup, cff_INTEGER, 1, pd->languageGroup);
	cffdict_input(dict, op_ExpansionFactor, cff_DOUBLE, 1, pd->expansionFactor);
	cffdict_input(dict, op_initialRandomSeed, cff_DOUBLE, 1, pd->initialRandomSeed);

	// op_nominalWidthX are currently not used
	cffdict_input(dict, op_defaultWidthX, cff_DOUBLE, 1, pd->defaultWidthX);
	cffdict_input(dict, op_nominalWidthX, cff_DOUBLE, 1, pd->nominalWidthX);
	return dict;
}

static int by_sid(cff_sid_entry *a, cff_sid_entry *b) {
	return a->sid - b->sid;
}
static caryll_buffer *callback_makestringindex(void *context, uint32_t i) {
	caryll_buffer **blobs = context;
	return blobs[i];
}
static caryll_buffer *cffstrings_to_indexblob(cff_sid_entry **h) {
	HASH_SORT(*h, by_sid);
	caryll_buffer **blobs;
	uint32_t n = HASH_COUNT(*h);
	NEW_N(blobs, n);
	uint32_t j = 0;
	cff_sid_entry *item, *tmp;
	HASH_ITER(hh, *h, item, tmp) {
		blobs[j] = bufnew();
		bufwrite_sds(blobs[j], item->str);
		HASH_DEL(*h, item);
		sdsfree(item->str);
		free(item);
		j++;
	}

	cff_Index *strings = cff_newIndexByCallback(blobs, n, callback_makestringindex);
	free(blobs);
	caryll_buffer *final_blob = cff_build_Index(*strings);
	cff_delete_Index(strings);
	final_blob->cursor = final_blob->size;
	return final_blob;
}

static caryll_buffer *cff_compile_nameindex(table_CFF *cff) {
	cff_Index *nameIndex = cff_new_Index();
	nameIndex->count = 1;
	nameIndex->offSize = 4;
	NEW_N(nameIndex->offset, 2);
	if (!cff->fontName) { cff->fontName = sdsnew("Caryll-CFF-FONT"); }
	nameIndex->offset[0] = 1;
	nameIndex->offset[1] = (uint32_t)(sdslen(cff->fontName) + 1);
	NEW_N(nameIndex->data, 1 + sdslen(cff->fontName));
	memcpy(nameIndex->data, cff->fontName, sdslen(cff->fontName));
	// CFF Name INDEX
	caryll_buffer *buf = cff_build_Index(*nameIndex);
	cff_delete_Index(nameIndex);
	if (cff->fontName) {
		sdsfree(cff->fontName);
		cff->fontName = NULL;
	}
	return buf;
}

static caryll_buffer *cff_make_charset(table_CFF *cff, table_glyf *glyf, cff_sid_entry **stringHash) {
	cff_Charset *charset;
	NEW_CLEAN(charset);
	if (glyf->numberGlyphs > 1) { // At least two glyphs
		charset->t = cff_CHARSET_FORMAT2;
		charset->s = 1; // One segment only
		charset->f2.format = 2;
		NEW(charset->f2.range2);
		if (cff->isCID) {
			charset->f2.range2[0].first = 1;
			charset->f2.range2[0].nleft = glyf->numberGlyphs - 2;
		} else {
			for (uint16_t j = 1; j < glyf->numberGlyphs; j++) {
				sidof(stringHash, glyf->glyphs[j]->name);
			}
			charset->f2.range2[0].first = sidof(stringHash, glyf->glyphs[1]->name);
			charset->f2.range2[0].nleft = glyf->numberGlyphs - 2;
		}
	} else {
		charset->t = cff_CHARSET_ISOADOBE;
	}
	caryll_buffer *c = cff_build_Charset(*charset);
	if (charset->t == cff_CHARSET_FORMAT2) { FREE(charset->f2.range2); }
	FREE(charset);
	return c;
}

static caryll_buffer *cff_make_fdselect(table_CFF *cff, table_glyf *glyf) {
	if (!cff->isCID) return bufnew();
	// We choose format 3 here
	uint32_t ranges = 1;
	uint8_t current = 0;
	cff_FDSelect *fds;
	NEW_CLEAN(fds);
	fds->t = cff_FDSELECT_UNSPECED;
	if (!glyf->numberGlyphs) goto done;
	uint8_t fdi0 = glyf->glyphs[0]->fdSelect.index;
	if (fdi0 > cff->fdArrayCount) fdi0 = 0;
	current = fdi0;
	for (uint16_t j = 1; j < glyf->numberGlyphs; j++) {
		uint8_t fdi = glyf->glyphs[j]->fdSelect.index;
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
		uint8_t fdi = glyf->glyphs[j]->fdSelect.index;
		if (fdi > cff->fdArrayCount) fdi = 0;
		if (glyf->glyphs[j]->fdSelect.index != current) {
			current = fdi;
			fds->s++;
			fds->f3.range3[fds->s].first = j;
			fds->f3.range3[fds->s].fd = current;
		}
	}
	fds->t = cff_FDSELECT_FORMAT3;
	fds->s = ranges;
	fds->f3.format = 3;
	fds->f3.nranges = ranges;
	fds->f3.sentinel = glyf->numberGlyphs;
done:;
	caryll_buffer *e = cff_build_FDSelect(*fds);
	cff_close_FDSelect(*fds);
	free(fds);
	return e;
}

typedef struct {
	table_CFF **fdArray;
	cff_sid_entry **stringHash;
} fdarray_compile_context;
static caryll_buffer *callback_makefd(void *_context, uint32_t i) {
	fdarray_compile_context *context = (fdarray_compile_context *)_context;
	cff_Dict *fd = cff_make_fd_dict(context->fdArray[i], context->stringHash);
	caryll_buffer *blob = cff_build_Dict(fd);
	bufwrite_bufdel(blob, cff_buildOffset(0xEEEEEEEE));
	bufwrite_bufdel(blob, cff_buildOffset(0xFFFFFFFF));
	bufwrite_bufdel(blob, cff_encodeCffOperator(op_Private));
	cff_delete_Dict(fd);
	return blob;
}
static cff_Index *cff_make_fdarray(uint16_t fdArrayCount, table_CFF **fdArray, cff_sid_entry **stringHash) {
	fdarray_compile_context context;
	context.fdArray = fdArray;
	context.stringHash = stringHash;

	return cff_newIndexByCallback(&context, fdArrayCount, callback_makefd);
}

static caryll_buffer *writecff_CIDKeyed(table_CFF *cff, table_glyf *glyf, const caryll_Options *options) {
	caryll_buffer *blob = bufnew();
	// The Strings hashtable
	cff_sid_entry *stringHash = NULL;

	// CFF Header
	caryll_buffer *h = cff_buildHeader();
	caryll_buffer *n = cff_compile_nameindex(cff);

	// cff top DICT
	cff_Dict *top = cff_make_fd_dict(cff, &stringHash);
	caryll_buffer *t = cff_build_Dict(top);
	cff_delete_Dict(top);
	// cff top PRIVATE
	cff_Dict *top_pd = cff_make_private_dict(cff->privateDict);
	caryll_buffer *p = cff_build_Dict(top_pd);
	cff_delete_Dict(top_pd);

	// FDSelect
	caryll_buffer *e = cff_make_fdselect(cff, glyf);

	// FDArray
	cff_Index *fdArrayIndex = NULL;
	caryll_buffer *r;
	if (cff->isCID) {
		fdArrayIndex = cff_make_fdarray(cff->fdArrayCount, cff->fdArray, &stringHash);
		r = cff_build_Index(*fdArrayIndex);
	} else {
		NEW_CLEAN(r);
	}

	// cff charset : Allocate string index terms for glyph names
	caryll_buffer *c = cff_make_charset(cff, glyf, &stringHash);

	// CFF String Index
	caryll_buffer *i = cffstrings_to_indexblob(&stringHash);

	// CFF Charstrings
	caryll_buffer *s;
	{
		cff_charstring_builder_context g2cContext;
		g2cContext.glyf = glyf;
		g2cContext.defaultWidth = cff->privateDict->defaultWidthX;
		g2cContext.nominalWidthX = cff->privateDict->nominalWidthX;
		g2cContext.options = options;
		g2cContext.graph = cff_new_Graph();
		s = cff_make_charstrings(&g2cContext);
		printRule(g2cContext.graph->root);
	}

	// Merge these data
	uint32_t additionalTopDictOpsSize = 0;
	uint32_t off = (uint32_t)(h->size + n->size + 11 + t->size);
	if (c->size != 0) additionalTopDictOpsSize += 6;  // op_charset
	if (e->size != 0) additionalTopDictOpsSize += 7;  // op_FDSelect
	if (s->size != 0) additionalTopDictOpsSize += 6;  // op_CharStrings
	if (p->size != 0) additionalTopDictOpsSize += 11; // op_Private
	if (r->size != 0) additionalTopDictOpsSize += 7;  // op_FDArray

	// Start building CFF table
	bufwrite_bufdel(blob, h); // header
	bufwrite_bufdel(blob, n); // name index
	int32_t delta_size = (uint32_t)(t->size + additionalTopDictOpsSize + 1);
	bufwrite_bufdel(blob, bufninit(11, 0, 1,   // top dict index headerone item
	                               4,          // four bytes per offset
	                               0, 0, 0, 1, // offset 1
	                               (delta_size >> 24) & 0xff, (delta_size >> 16) & 0xff, (delta_size >> 8) & 0xff,
	                               delta_size & 0xff) // offset 2
	                );

	bufwrite_bufdel(blob, t); // top dict body

	// top dict offsets
	off += additionalTopDictOpsSize + i->size + 3;
	if (c->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_charset));
		off += c->size;
	}
	if (e->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_FDSelect));
		off += e->size;
	}
	if (s->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_CharStrings));
		off += s->size;
	}
	if (p->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset((uint32_t)(p->size)));
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_Private));
		off += p->size;
	}
	if (r->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_FDArray));
		off += r->size;
	}

	bufwrite_bufdel(blob, i);                    // string index
	bufwrite_bufdel(blob, bufninit(3, 0, 0, 0)); // gsubr. Currently empty
	bufwrite_bufdel(blob, c);                    // charset
	bufwrite_bufdel(blob, e);                    // fdselect
	bufwrite_bufdel(blob, s);                    // charstring
	bufwrite_bufdel(blob, p);                    // top private
	if (cff->isCID) {                            // fdarray and fdarray privates
		uint32_t fdArrayPrivatesStartOffset = off;
		caryll_buffer **fdArrayPrivates;
		NEW_N(fdArrayPrivates, cff->fdArrayCount);
		for (uint16_t j = 0; j < cff->fdArrayCount; j++) {
			cff_Dict *pd = cff_make_private_dict(cff->fdArray[j]->privateDict);
			caryll_buffer *p = cff_build_Dict(pd);
			cff_delete_Dict(pd);
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
		buffree(r); // fdarray
		r = cff_build_Index(*fdArrayIndex);
		cff_delete_Index(fdArrayIndex);
		bufwrite_bufdel(blob, r);
		for (uint16_t j = 0; j < cff->fdArrayCount; j++) {
			bufwrite_bufdel(blob, fdArrayPrivates[j]);
		}
		free(fdArrayPrivates);
	} else {
		bufwrite_bufdel(blob, r);
	}
	// finish
	return blob;
}

caryll_buffer *table_build_CFF(table_CFFAndGlyf cffAndGlyf, const caryll_Options *options) {
	return writecff_CIDKeyed(cffAndGlyf.meta, cffAndGlyf.glyphs, options);
}
