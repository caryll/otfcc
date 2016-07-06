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
		context->g->stemV[context->definedVStems].isEdge = width < 0;
		context->definedVStems += 1;
	} else {
		context->g->stemH[context->definedHStems].position = position;
		context->g->stemH[context->definedHStems].width = width;
		context->g->stemH[context->definedHStems].isEdge = width < 0;
		context->definedHStems += 1;
	}
}
static void callback_draw_setmask(void *_context, bool isContourMask, bool *maskArray) {
	outline_builder_context *context = (outline_builder_context *)_context;
	glyf_postscript_hint_mask *mask =
	    &(isContourMask ? context->g->contourMasks
	                    : context->g->hintMasks)[isContourMask ? context->definedContourMasks
	                                                           : context->definedHintMasks];
	mask->pointsBefore = context->pointsDefined;
	for (uint16_t j = 0; j < 0x100; j++) {
		mask->maskH[j] = j < context->g->numberOfStemH ? maskArray[j] : 0;
		mask->maskV[j] =
		    j < context->g->numberOfStemV ? maskArray[j + context->g->numberOfStemH] : 0;
	}

	free(maskArray);
	if (isContourMask) {
		context->definedContourMasks += 1;
	} else {
		context->definedHintMasks += 1;
	}
}

static void buildOutline(uint16_t i, cff_parse_context *context) {
	CFF_File *f = context->cffFile;
	glyf_glyph *g = caryll_new_glyf_glyph();
	context->glyphs->glyphs[i] = g;

	CFF_INDEX localSubrs;
	CFF_Stack stack;

	stack.index = 0;
	stack.stem = 0;

	outline_builder_context bc = {g, 0, 0, 0.0, 0.0, 0, 0, 0, 0, 0};
	cff_outline_builder_interface pass1 = {NULL, callback_count_contour, NULL, NULL, NULL, NULL};
	cff_outline_builder_interface pass2 = {NULL,
	                                       callback_countpoint_next_contour,
	                                       callback_countpoint_lineto,
	                                       callback_countpoint_curveto,
	                                       callback_countpoint_sethint,
	                                       callback_countpoint_setmask};
	cff_outline_builder_interface pass3 = {callback_draw_setwidth, callback_draw_next_contour,
	                                       callback_draw_lineto,   callback_draw_curveto,
	                                       callback_draw_sethint,  callback_draw_setmask};

	uint8_t fd = 0;
	if (f->fdselect.t != CFF_FDSELECT_UNSPECED)
		fd = parse_subr(i, f->raw_data, f->font_dict, f->fdselect, &localSubrs);
	else
		fd = parse_subr(i, f->raw_data, f->top_dict, f->fdselect, &localSubrs);

	g->fdIndex = fd;
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
	parse_outline_callback(charStringPtr, charStringLength, f->global_subr, localSubrs, &stack, &bc,
	                       pass1);
	NEW_N(g->contours, g->numberOfContours);

	// PASS 2 : Count points
	stack.index = 0;
	stack.stem = 0;
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
	parse_outline_callback(charStringPtr, charStringLength, f->global_subr, localSubrs, &stack, &bc,
	                       pass3);

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

table_CFF *caryll_CFF_from_json(json_value *root, caryll_dump_options *dumpopts) { return NULL; }

caryll_buffer *caryll_write_CFF(caryll_cff_parse_result cffAndGlyf) { return NULL; }
