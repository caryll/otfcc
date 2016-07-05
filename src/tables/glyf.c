#include "glyf.h"
#include <math.h>

typedef enum { MASK_ON_CURVE = 1 } glyf_oncurve_mask;

glyf_glyph *caryll_new_glyf_glyph() {
	glyf_glyph *g = malloc(sizeof(glyf_glyph));
	g->numberOfContours = 0;
	g->numberOfReferences = 0;
	g->instructionsLength = 0;
	g->instructions = NULL;
	g->contours = NULL;
	g->references = NULL;
	g->advanceWidth = 0;
	g->name = NULL;
	g->advanceHeight = 0;
	g->verticalOrigin = 0;
	g->numberOfStemH = 0;
	g->numberOfStemV = 0;
	g->numberOfHintMasks = 0;
	g->numberOfContourMasks = 0;
	g->stemH = NULL;
	g->stemV = NULL;
	g->hintMasks = NULL;
	g->contourMasks = NULL;

	g->stat.xMin = 0;
	g->stat.xMax = 0;
	g->stat.yMin = 0;
	g->stat.yMax = 0;
	g->stat.nestDepth = 0;
	g->stat.nPoints = 0;
	g->stat.nContours = 0;
	g->stat.nCompositePoints = 0;
	g->stat.nCompositeContours = 0;
	return g;
}

static INLINE glyf_point *next_point(glyf_contour *contours, uint16_t *cc, uint16_t *cp) {
	if (*cp >= contours[*cc].pointsCount) {
		*cp = 0;
		*cc += 1;
	}
	return &contours[*cc].points[(*cp)++];
}

static INLINE glyf_glyph *caryll_read_simple_glyph(font_file_pointer start,
                                                   uint16_t numberOfContours) {
	glyf_glyph *g = caryll_new_glyf_glyph();
	g->numberOfContours = numberOfContours;
	g->numberOfReferences = 0;

	glyf_contour *contours = (glyf_contour *)malloc(sizeof(glyf_contour) * numberOfContours);
	uint16_t lastPointIndex = 0;
	for (uint16_t j = 0; j < numberOfContours; j++) {
		uint16_t lastPointInCurrentContour = read_16u(start + 2 * j);
		contours[j].pointsCount = lastPointInCurrentContour - lastPointIndex + 1;
		contours[j].points = (glyf_point *)malloc(sizeof(glyf_point) * contours[j].pointsCount);
		lastPointIndex = lastPointInCurrentContour + 1;
	}

	uint16_t instructionLength = read_16u(start + 2 * numberOfContours);
	uint8_t *instructions = NULL;
	if (instructionLength > 0) {
		instructions = (font_file_pointer)malloc(sizeof(uint8_t) * instructionLength);
		memcpy(instructions, start + 2 * numberOfContours + 2, sizeof(uint8_t) * instructionLength);
	}
	g->instructionsLength = instructionLength;
	g->instructions = instructions;

	// read flags
	uint16_t pointsInGlyph = lastPointIndex;
	// There are repeating entries in the flags list, we will fill out the
	// result
	font_file_pointer flags = (uint8_t *)malloc(sizeof(uint8_t) * pointsInGlyph);
	font_file_pointer flagStart = start + 2 * numberOfContours + 2 + instructionLength;
	uint16_t flagsReadSofar = 0;
	uint16_t flagBytesReadSofar = 0;

	uint16_t currentContour = 0;
	uint16_t currentContourPointIndex = 0;
	while (flagsReadSofar < pointsInGlyph) {
		uint8_t flag = flagStart[flagBytesReadSofar];
		flags[flagsReadSofar] = flag;
		flagBytesReadSofar += 1;
		flagsReadSofar += 1;
		next_point(contours, &currentContour, &currentContourPointIndex)->onCurve =
		    (flag & GLYF_FLAG_ON_CURVE);
		if (flag & GLYF_FLAG_REPEAT) { // repeating flag
			uint8_t repeat = flagStart[flagBytesReadSofar];
			flagBytesReadSofar += 1;
			for (uint8_t j = 0; j < repeat; j++) {
				flags[flagsReadSofar + j] = flag;
				next_point(contours, &currentContour, &currentContourPointIndex)->onCurve =
				    (flag & GLYF_FLAG_ON_CURVE);
			}
			flagsReadSofar += repeat;
		}
	}

	// read X coordinates
	font_file_pointer coordinatesStart = flagStart + flagBytesReadSofar;
	uint32_t coordinatesOffset = 0;
	uint16_t coordinatesRead = 0;
	currentContour = 0;
	currentContourPointIndex = 0;
	while (coordinatesRead < pointsInGlyph) {
		uint8_t flag = flags[coordinatesRead];
		int16_t x;
		if (flag & GLYF_FLAG_X_SHORT) {
			x = (flag & GLYF_FLAG_POSITIVE_X ? 1 : -1) *
			    read_8u(coordinatesStart + coordinatesOffset);
			coordinatesOffset += 1;
		} else {
			if (flag & GLYF_FLAG_SAME_X) {
				x = 0;
			} else {
				x = read_16s(coordinatesStart + coordinatesOffset);
				coordinatesOffset += 2;
			}
		}
		next_point(contours, &currentContour, &currentContourPointIndex)->x = x;
		coordinatesRead += 1;
	}
	// read Y, identical to X
	coordinatesRead = 0;
	currentContour = 0;
	currentContourPointIndex = 0;
	while (coordinatesRead < pointsInGlyph) {
		uint8_t flag = flags[coordinatesRead];
		int16_t y;
		if (flag & GLYF_FLAG_Y_SHORT) {
			y = (flag & GLYF_FLAG_POSITIVE_Y ? 1 : -1) *
			    read_8u(coordinatesStart + coordinatesOffset);
			coordinatesOffset += 1;
		} else {
			if (flag & GLYF_FLAG_SAME_Y) {
				y = 0;
			} else {
				y = read_16s(coordinatesStart + coordinatesOffset);
				coordinatesOffset += 2;
			}
		}
		next_point(contours, &currentContour, &currentContourPointIndex)->y = y;
		coordinatesRead += 1;
	}
	free(flags);
	// turn deltas to absolute coordiantes
	float cx = 0;
	float cy = 0;
	for (uint16_t j = 0; j < numberOfContours; j++) {
		for (uint16_t k = 0; k < contours[j].pointsCount; k++) {
			cx += contours[j].points[k].x;
			contours[j].points[k].x = cx;
			cy += contours[j].points[k].y;
			contours[j].points[k].y = cy;
		}
	}
	g->contours = contours;
	return g;
}

static INLINE glyf_glyph *caryll_read_composite_glyph(font_file_pointer start) {
	glyf_glyph *g = caryll_new_glyf_glyph();
	g->numberOfContours = 0;
	// pass 1, read references quantity
	uint16_t flags;
	uint16_t numberOfReferences = 0;
	uint32_t offset = 0;
	bool glyphHasInstruction = false;
	do {
		flags = read_16u(start + offset);
		offset += 4; // flags & index
		numberOfReferences += 1;
		if (flags & ARG_1_AND_2_ARE_WORDS) {
			offset += 4;
		} else {
			offset += 2;
		}
		if (flags & WE_HAVE_A_SCALE) {
			offset += 2;
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			offset += 4;
		} else if (flags & WE_HAVE_A_TWO_BY_TWO) {
			offset += 8;
		}
		if (flags & WE_HAVE_INSTRUCTIONS) { glyphHasInstruction = true; }
	} while (flags & MORE_COMPONENTS);

	// pass 2, read references
	g->numberOfReferences = numberOfReferences;
	g->references = (glyf_reference *)malloc(sizeof(glyf_reference) * numberOfReferences);
	offset = 0;
	for (uint16_t j = 0; j < numberOfReferences; j++) {
		flags = read_16u(start + offset);
		uint16_t index = read_16u(start + offset + 2);
		int16_t x = 0;
		int16_t y = 0;

		offset += 4; // flags & index
		if (flags & ARG_1_AND_2_ARE_WORDS) {
			x = read_16s(start + offset);
			y = read_16s(start + offset + 2);
			offset += 4;
		} else {
			x = read_8s(start + offset);
			y = read_8s(start + offset + 1);
			offset += 2;
		}
		float a = 1.0;
		float b = 0.0;
		float c = 0.0;
		float d = 1.0;
		if (flags & WE_HAVE_A_SCALE) {
			a = d = caryll_from_f2dot14(read_16s(start + offset));
			offset += 2;
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			a = caryll_from_f2dot14(read_16s(start + offset));
			d = caryll_from_f2dot14(read_16s(start + offset + 2));
			offset += 4;
		} else if (flags & WE_HAVE_A_TWO_BY_TWO) {
			a = caryll_from_f2dot14(read_16s(start + offset));
			b = caryll_from_f2dot14(read_16s(start + offset + 2));
			c = caryll_from_f2dot14(read_16s(start + offset + 4));
			d = caryll_from_f2dot14(read_16s(start + offset + 2));
			offset += 8;
		}
		g->references[j].glyph.gid = index;
		g->references[j].a = a;
		g->references[j].b = b;
		g->references[j].c = c;
		g->references[j].d = d;
		g->references[j].x = x;
		g->references[j].y = y;
		g->references[j].roundToGrid = !!(flags & ROUND_XY_TO_GRID);
		g->references[j].useMyMetrics = !!(flags & USE_MY_METRICS);
	}
	if (glyphHasInstruction) {
		uint16_t instructionLength = read_16u(start + offset);
		font_file_pointer instructions = NULL;
		if (instructionLength > 0) {
			instructions = (font_file_pointer)malloc(sizeof(uint8_t) * instructionLength);
			memcpy(instructions, start + offset + 2, sizeof(uint8_t) * instructionLength);
		}
		g->instructionsLength = instructionLength;
		g->instructions = instructions;
	} else {
		g->instructionsLength = 0;
		g->instructions = NULL;
	}

	return g;
}

static INLINE glyf_glyph *caryll_read_glyph(font_file_pointer data, uint32_t offset) {
	font_file_pointer start = data + offset;
	int16_t numberOfContours = read_16u(start);
	glyf_glyph *g;
	if (numberOfContours > 0) {
		g = caryll_read_simple_glyph(start + 10, numberOfContours);
	} else {
		g = caryll_read_composite_glyph(start + 10);
	}
	g->stat.xMin = read_16s(start + 2);
	g->stat.yMin = read_16s(start + 4);
	g->stat.xMax = read_16s(start + 6);
	g->stat.yMax = read_16s(start + 8);
	return g;
}

table_glyf *caryll_read_glyf(caryll_packet packet, table_head *head, table_maxp *maxp) {
	if (head == NULL || maxp == NULL) return NULL;
	uint32_t *offsets = NULL;
	table_glyf *glyf = NULL;

	uint16_t locaIsLong = head->indexToLocFormat;
	uint16_t numGlyphs = maxp->numGlyphs;
	offsets = (uint32_t *)malloc(sizeof(uint32_t) * (numGlyphs + 1));
	if (!offsets) goto ABSENT;
	bool foundLoca = false;

	// read loca
	FOR_TABLE('loca', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 2 * numGlyphs + 2) goto LOCA_CORRUPTED;
		for (uint32_t j = 0; j < numGlyphs + 1; j++) {
			if (locaIsLong) {
				offsets[j] = read_32u(data + j * 4);
			} else {
				offsets[j] = read_16u(data + j * 2) * 2;
			}
			if (j > 0 && offsets[j] < offsets[j - 1]) goto LOCA_CORRUPTED;
		}
		foundLoca = true;
		break;
	LOCA_CORRUPTED:
		fprintf(stderr, "table 'loca' corrupted.\n");
		if (offsets) { free(offsets), offsets = NULL; }
		continue;
	}
	if (!foundLoca) goto ABSENT;

	// read glyf
	FOR_TABLE('glyf', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < offsets[numGlyphs]) goto GLYF_CORRUPTED;

		glyf = malloc(sizeof(table_glyf));
		glyf->numberGlyphs = numGlyphs;
		glyf->glyphs = malloc(sizeof(glyf_glyph) * numGlyphs);

		for (uint16_t j = 0; j < numGlyphs; j++) {
			if (offsets[j] < offsets[j + 1]) { // non-space glyph
				glyf->glyphs[j] = caryll_read_glyph(data, offsets[j]);
			} else { // space glyph
				glyf->glyphs[j] = caryll_new_glyf_glyph();
			}
		}
		goto PRESENT;
	GLYF_CORRUPTED:
		fprintf(stderr, "table 'glyf' corrupted.\n");
		if (glyf) { caryll_delete_glyf(glyf), glyf = NULL; }
	}
	goto ABSENT;

PRESENT:
	if (offsets) { free(offsets), offsets = NULL; }
	return glyf;

ABSENT:
	if (offsets) { free(offsets), offsets = NULL; }
	if (glyf) { free(glyf), glyf = NULL; }
	return NULL;
}

static void caryll_delete_glyf_glyph(glyf_glyph *g) {
	if (!g) return;
	if (g->numberOfContours > 0 && g->contours != NULL) {
		for (uint16_t k = 0; k < g->numberOfContours; k++) {
			if (g->contours[k].points) free(g->contours[k].points);
		}
		free(g->contours);
	}
	if (g->numberOfReferences > 0 && g->references != NULL) {
		for (uint16_t k = 0; k < g->numberOfReferences; k++) { g->references[k].glyph.name = NULL; }
		free(g->references);
	}
	if (g->instructions) { free(g->instructions); }
	if (g->stemH) FREE(g->stemH);
	if (g->stemV) FREE(g->stemV);
	if (g->hintMasks) FREE(g->hintMasks);
	if (g->contourMasks) FREE(g->contourMasks);
	g->name = NULL;
	free(g);
}
void caryll_delete_glyf(table_glyf *table) {
	if (table->glyphs) {
		for (uint16_t j = 0; j < table->numberGlyphs; j++) {
			caryll_delete_glyf_glyph(table->glyphs[j]);
		}
		free(table->glyphs);
	}
	free(table);
}

// to json
static INLINE json_value *coord_to_json(float z) {
	if (roundf(z) == z) {
		return json_integer_new(z);
	} else {
		return json_double_new(z);
	}
}
static INLINE json_value *glyf_glyph_contours_to_json(glyf_glyph *g,
                                                      caryll_dump_options *dumpopts) {
	json_value *contours = json_array_new(g->numberOfContours);
	for (uint16_t k = 0; k < g->numberOfContours; k++) {
		glyf_contour c = g->contours[k];
		json_value *contour = json_array_new(c.pointsCount);
		for (uint16_t m = 0; m < c.pointsCount; m++) {
			json_value *point = json_object_new(3);
			json_object_push(point, "x", coord_to_json(c.points[m].x));
			json_object_push(point, "y", coord_to_json(c.points[m].y));
			json_object_push(point, "on", json_boolean_new(c.points[m].onCurve & MASK_ON_CURVE));
			json_array_push(contour, point);
		}
		json_array_push(contours, contour);
	}
	return contours;
}
static INLINE json_value *glyf_glyph_references_to_json(glyf_glyph *g,
                                                        caryll_dump_options *dumpopts) {
	json_value *references = json_array_new(g->numberOfReferences);
	for (uint16_t k = 0; k < g->numberOfReferences; k++) {
		glyf_reference *r = &(g->references[k]);
		json_value *ref = json_object_new(9);
		json_object_push(ref, "glyph",
		                 json_string_new_length(sdslen(r->glyph.name), r->glyph.name));
		json_object_push(ref, "x", coord_to_json(r->x));
		json_object_push(ref, "y", coord_to_json(r->y));
		json_object_push(ref, "a", coord_to_json(r->a));
		json_object_push(ref, "b", coord_to_json(r->b));
		json_object_push(ref, "c", coord_to_json(r->c));
		json_object_push(ref, "d", coord_to_json(r->d));
		if (r->roundToGrid) { json_object_push(ref, "roundToGrid", json_boolean_new(true)); }
		if (r->useMyMetrics) { json_object_push(ref, "useMyMetrics", json_boolean_new(true)); }
		json_array_push(references, ref);
	}
	return references;
}
static INLINE json_value *glyf_glyph_stemdefs_to_json(glyf_postscript_hint_stemdef *stems,
                                                      uint16_t count) {
	json_value *a = json_array_new(count);
	for (uint16_t j = 0; j < count; j++) {
		json_value *stem = json_object_new(3);
		json_object_push(stem, "position", coord_to_json(stems[j].position));
		json_object_push(stem, "width", coord_to_json(stems[j].width));
		json_object_push(stem, "isEdge", json_boolean_new(stems[j].isEdge));
		json_array_push(a, stem);
	}
	return a;
}
static INLINE json_value *glyf_glyph_maskdefs_to_json(glyf_postscript_hint_mask *masks,
                                                      uint16_t count, uint16_t nh, uint16_t nv) {
	json_value *a = json_array_new(count);
	for (uint16_t j = 0; j < count; j++) {
		json_value *mask = json_object_new(3);
		json_object_push(mask, "pointsBefore", json_integer_new(masks[j].pointsBefore));
		json_value *h = json_array_new(nh);
		for (uint16_t k = 0; k < nh; k++) {
			json_array_push(h, json_boolean_new(masks[j].maskH[k]));
		}
		json_object_push(mask, "maskH", h);
		json_value *v = json_array_new(nv);
		for (uint16_t k = 0; k < nv; k++) {
			json_array_push(v, json_boolean_new(masks[j].maskV[k]));
		}
		json_object_push(mask, "maskV", v);
		json_array_push(a, mask);
	}
	return a;
}

static INLINE json_value *glyf_glyph_to_json(glyf_glyph *g, caryll_dump_options *dumpopts) {
	json_value *glyph = json_object_new(7);
	json_object_push(glyph, "advanceWidth", json_integer_new(g->advanceWidth));
	if (dumpopts->has_vertical_metrics) {
		json_object_push(glyph, "advanceHeight", json_integer_new(g->advanceHeight));
		json_object_push(glyph, "verticalOrigin", json_integer_new(g->verticalOrigin));
	}
	if (g->contours) {
		json_object_push(glyph, "contours", preserialize(glyf_glyph_contours_to_json(g, dumpopts)));
	}
	if (g->references) {
		json_object_push(glyph, "references",
		                 preserialize(glyf_glyph_references_to_json(g, dumpopts)));
	}
	if (!dumpopts->ignore_hints && g->instructionsLength) {
		json_object_push(glyph, "instructions",
		                 instr_to_json(g->instructions, g->instructionsLength, dumpopts));
	}
	if (!dumpopts->ignore_hints && g->stemH) {
		json_object_push(glyph, "stemH",
		                 preserialize(glyf_glyph_stemdefs_to_json(g->stemH, g->numberOfStemH)));
	}
	if (!dumpopts->ignore_hints && g->stemV) {
		json_object_push(glyph, "stemV",
		                 preserialize(glyf_glyph_stemdefs_to_json(g->stemV, g->numberOfStemV)));
	}
	if (!dumpopts->ignore_hints && g->hintMasks) {
		json_object_push(glyph, "hintMasks", preserialize(glyf_glyph_maskdefs_to_json(
		                                         g->hintMasks, g->numberOfHintMasks,
		                                         g->numberOfStemH, g->numberOfStemV)));
	}
	if (!dumpopts->ignore_hints && g->contourMasks) {
		json_object_push(glyph, "contourMasks", preserialize(glyf_glyph_maskdefs_to_json(
		                                            g->contourMasks, g->numberOfContourMasks,
		                                            g->numberOfStemH, g->numberOfStemV)));
	}
	return glyph;
}
void caryll_glyphorder_to_json(table_glyf *table, json_value *root) {
	if (!table) return;
	json_value *order = json_array_new(table->numberGlyphs);
	for (uint16_t j = 0; j < table->numberGlyphs; j++) {
		json_array_push(
		    order, json_string_new_length(sdslen(table->glyphs[j]->name), table->glyphs[j]->name));
	}
	json_object_push(root, "glyph_order", preserialize(order));
}
void caryll_glyf_to_json(table_glyf *table, json_value *root, caryll_dump_options *dumpopts) {
	if (!table) return;
	json_value *glyf = json_object_new(table->numberGlyphs);
	for (uint16_t j = 0; j < table->numberGlyphs; j++) {
		glyf_glyph *g = table->glyphs[j];
		json_object_push(glyf, g->name, glyf_glyph_to_json(g, dumpopts));
	}
	json_object_push(root, "glyf", glyf);

	if (!dumpopts->ignore_glyph_order) caryll_glyphorder_to_json(table, root);
}

// from json
static INLINE void glyf_point_from_json(glyf_point *point, json_value *pointdump) {
	point->x = json_obj_getnum(pointdump, "x");
	point->y = json_obj_getnum(pointdump, "y");
	point->onCurve = 0;
	if (json_obj_getbool(pointdump, "on")) { point->onCurve |= MASK_ON_CURVE; }
}
static INLINE void glyf_reference_from_json(glyf_reference *ref, json_value *refdump) {
	json_value *_gname = json_obj_get_type(refdump, "glyph", json_string);
	if (_gname) {
		ref->glyph.name = sdsnewlen(_gname->u.string.ptr, _gname->u.string.length);
		ref->x = json_obj_getnum_fallback(refdump, "x", 0.0);
		ref->y = json_obj_getnum_fallback(refdump, "y", 0.0);
		ref->a = json_obj_getnum_fallback(refdump, "a", 1.0);
		ref->b = json_obj_getnum_fallback(refdump, "b", 0.0);
		ref->c = json_obj_getnum_fallback(refdump, "c", 0.0);
		ref->d = json_obj_getnum_fallback(refdump, "d", 1.0);
		ref->roundToGrid = json_obj_getbool(refdump, "roundToGrid");
		ref->useMyMetrics = json_obj_getbool(refdump, "useMyMetrics");
	} else {
		// Invalid glyph references
		ref->glyph.name = NULL;
		ref->x = 0.0;
		ref->y = 0.0;
		ref->a = 1.0;
		ref->b = 0.0;
		ref->c = 0.0;
		ref->d = 1.0;
		ref->roundToGrid = false;
		ref->useMyMetrics = false;
	}
}
static INLINE void glyf_contours_from_json(json_value *col, glyf_glyph *g) {
	if (!col) {
		g->numberOfContours = 0;
		g->contours = NULL;
		return;
	}
	g->numberOfContours = col->u.array.length;
	g->contours = malloc(g->numberOfContours * sizeof(glyf_contour));
	for (uint16_t j = 0; j < g->numberOfContours; j++) {
		json_value *contourdump = col->u.array.values[j];
		if (contourdump && contourdump->type == json_array) {
			g->contours[j].pointsCount = contourdump->u.array.length;
			g->contours[j].points = malloc(g->contours[j].pointsCount * sizeof(glyf_point));
			for (uint16_t k = 0; k < g->contours[j].pointsCount; k++) {
				glyf_point_from_json(&(g->contours[j].points[k]), contourdump->u.array.values[k]);
			}
		} else {
			g->contours[j].pointsCount = 0;
			g->contours[j].points = NULL;
		}
	}
}
static INLINE void glyf_references_from_json(json_value *col, glyf_glyph *g) {
	if (!col) {
		g->numberOfReferences = 0;
		g->references = NULL;
		return;
	}
	g->numberOfReferences = col->u.array.length;
	g->references = malloc(g->numberOfReferences * sizeof(glyf_reference));
	for (uint16_t j = 0; j < g->numberOfReferences; j++) {
		glyf_reference_from_json(&(g->references[j]), col->u.array.values[j]);
	}
}

static INLINE void makeInstrsForGlyph(void *_g, uint8_t *instrs, uint32_t len) {
	glyf_glyph *g = (glyf_glyph *)_g;
	g->instructionsLength = len;
	g->instructions = instrs;
}
static INLINE void wrongInstrsForGlyph(void *_g, char *reason, int pos) {
	glyf_glyph *g = (glyf_glyph *)_g;
	fprintf(stderr, "[OTFCC] TrueType instructions parse error : %s, at %d in /%s\n", reason, pos,
	        g->name);
}

static INLINE glyf_glyph *caryll_glyf_glyph_from_json(json_value *glyphdump,
                                                      glyph_order_entry *order_entry,
                                                      caryll_dump_options *dumpopts) {
	glyf_glyph *g = caryll_new_glyf_glyph();
	g->name = order_entry->name;
	g->advanceWidth = json_obj_getint(glyphdump, "advanceWidth");
	g->advanceHeight = json_obj_getint(glyphdump, "advanceHeight");
	g->verticalOrigin = json_obj_getint(glyphdump, "verticalOrigin");
	glyf_contours_from_json(json_obj_get_type(glyphdump, "contours", json_array), g);
	glyf_references_from_json(json_obj_get_type(glyphdump, "references", json_array), g);
	if (!dumpopts->ignore_hints) {
		instr_from_json(json_obj_get(glyphdump, "instructions"), g, makeInstrsForGlyph,
		                wrongInstrsForGlyph);
	} else {
		g->instructionsLength = 0;
		g->instructions = NULL;
	}
	return g;
}

table_glyf *caryll_glyf_from_json(json_value *root, glyph_order_hash glyph_order,
                                  caryll_dump_options *dumpopts) {
	if (root->type != json_object || !glyph_order) return NULL;
	table_glyf *glyf = NULL;
	json_value *table;
	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		uint16_t numGlyphs = table->u.object.length;
		glyf = malloc(sizeof(table_glyf));
		glyf->numberGlyphs = numGlyphs;
		glyf->glyphs = calloc(numGlyphs, sizeof(glyf_glyph *));
		for (uint16_t j = 0; j < numGlyphs; j++) {
			sds gname =
			    sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
			json_value *glyphdump = table->u.object.values[j].value;
			glyph_order_entry *order_entry;
			HASH_FIND_STR(glyph_order, gname, order_entry);
			if (glyphdump->type == json_object && order_entry && !glyf->glyphs[order_entry->gid]) {
				glyf->glyphs[order_entry->gid] =
				    caryll_glyf_glyph_from_json(glyphdump, order_entry, dumpopts);
			}
			sdsfree(gname);
		}
		return glyf;
	}
	return NULL;
}

caryll_buffer *shrinkFlags(caryll_buffer *flags) {
	if (!buflen(flags)) return (flags);
	caryll_buffer *shrunk = bufnew();
	bufwrite8(shrunk, flags->s[0]);
	int repeating = 0;
	for (size_t j = 1; j < buflen(flags); j++) {
		if (flags->s[j] == flags->s[j - 1]) {
			if (repeating && repeating < 0xFE) {
				shrunk->s[shrunk->cursor - 1] += 1;
				repeating += 1;
			} else if (repeating == 0) {
				shrunk->s[shrunk->cursor - 1] |= GLYF_FLAG_REPEAT;
				bufwrite8(shrunk, 1);
				repeating += 1;
			} else {
				repeating = 0;
				bufwrite8(shrunk, flags->s[j]);
			}
		} else {
			repeating = 0;
			bufwrite8(shrunk, flags->s[j]);
		}
	}
	buffree(flags);
	return shrunk;
}

// serialize
#define EPSILON (1e-5)
static INLINE void glyf_write_simple(glyf_glyph *g, caryll_buffer *gbuf) {
	caryll_buffer *flags = bufnew();
	caryll_buffer *xs = bufnew();
	caryll_buffer *ys = bufnew();

	bufwrite16b(gbuf, g->numberOfContours);
	bufwrite16b(gbuf, (int16_t)g->stat.xMin);
	bufwrite16b(gbuf, (int16_t)g->stat.yMin);
	bufwrite16b(gbuf, (int16_t)g->stat.xMax);
	bufwrite16b(gbuf, (int16_t)g->stat.yMax);

	// endPtsOfContours[n]
	uint16_t ptid = 0;
	for (uint16_t j = 0; j < g->numberOfContours; j++) {
		ptid += g->contours[j].pointsCount;
		bufwrite16b(gbuf, ptid - 1);
	}

	// instructions
	bufwrite16b(gbuf, g->instructionsLength);
	if (g->instructions) bufwrite_bytes(gbuf, g->instructionsLength, g->instructions);

	// flags and points
	bufclear(flags);
	bufclear(xs);
	bufclear(ys);
	float cx = 0;
	float cy = 0;
	for (uint16_t cj = 0; cj < g->numberOfContours; cj++) {
		for (uint16_t k = 0; k < g->contours[cj].pointsCount; k++) {
			glyf_point *p = &(g->contours[cj].points[k]);
			uint8_t flag = (p->onCurve & MASK_ON_CURVE) ? GLYF_FLAG_ON_CURVE : 0;
			int16_t dx = p->x - cx;
			int16_t dy = p->y - cy;
			if (dx == 0) {
				flag |= GLYF_FLAG_SAME_X;
			} else if (dx >= -0xFF && dx <= 0xFF) {
				flag |= GLYF_FLAG_X_SHORT;
				if (dx > 0) {
					flag |= GLYF_FLAG_POSITIVE_X;
					bufwrite8(xs, dx);
				} else {
					bufwrite8(xs, -dx);
				}
			} else {
				bufwrite16b(xs, dx);
			}

			if (dy == 0) {
				flag |= GLYF_FLAG_SAME_Y;
			} else if (dy >= -0xFF && dy <= 0xFF) {
				flag |= GLYF_FLAG_Y_SHORT;
				if (dy > 0) {
					flag |= GLYF_FLAG_POSITIVE_Y;
					bufwrite8(ys, dy);
				} else {
					bufwrite8(ys, -dy);
				}
			} else {
				bufwrite16b(ys, dy);
			}
			bufwrite8(flags, flag);
			cx = p->x;
			cy = p->y;
		}
	}
	flags = shrinkFlags(flags);
	bufwrite_buf(gbuf, flags);
	bufwrite_buf(gbuf, xs);
	bufwrite_buf(gbuf, ys);

	buffree(flags);
	buffree(xs);
	buffree(ys);
}
static INLINE void glyf_write_composite(glyf_glyph *g, caryll_buffer *gbuf) {
	bufwrite16b(gbuf, (-1));
	bufwrite16b(gbuf, (int16_t)g->stat.xMin);
	bufwrite16b(gbuf, (int16_t)g->stat.yMin);
	bufwrite16b(gbuf, (int16_t)g->stat.xMax);
	bufwrite16b(gbuf, (int16_t)g->stat.yMax);
	for (uint16_t rj = 0; rj < g->numberOfReferences; rj++) {
		glyf_reference *r = &(g->references[rj]);
		uint16_t flags =
		    ARGS_ARE_XY_VALUES |
		    (rj < g->numberOfReferences - 1 ? MORE_COMPONENTS
		                                    : g->instructionsLength > 0 ? WE_HAVE_INSTRUCTIONS : 0);
		int16_t arg1 = r->x;
		int16_t arg2 = r->y;
		if (!(arg1 < 128 && arg1 >= -128 && arg2 < 128 && arg2 >= -128))
			flags |= ARG_1_AND_2_ARE_WORDS;
		if (fabsf(r->b) > EPSILON || fabsf(r->c) > EPSILON) {
			flags |= WE_HAVE_A_TWO_BY_TWO;
		} else if (fabsf(r->a - 1) > EPSILON || fabsf(r->d - 1) > EPSILON) {
			if (fabsf(r->a - r->d) > EPSILON) {
				flags |= WE_HAVE_AN_X_AND_Y_SCALE;
			} else {
				flags |= WE_HAVE_A_SCALE;
			}
		}
		bufwrite16b(gbuf, flags);
		bufwrite16b(gbuf, r->glyph.gid);
		if (flags & ARG_1_AND_2_ARE_WORDS) {
			bufwrite16b(gbuf, arg1);
			bufwrite16b(gbuf, arg2);
		} else {
			bufwrite8(gbuf, arg1);
			bufwrite8(gbuf, arg2);
		}
		if (flags & WE_HAVE_A_SCALE) {
			bufwrite16b(gbuf, caryll_to_f2dot14(r->a));
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			bufwrite16b(gbuf, caryll_to_f2dot14(r->a));
			bufwrite16b(gbuf, caryll_to_f2dot14(r->d));
		} else if (flags & WE_HAVE_A_TWO_BY_TWO) {
			bufwrite16b(gbuf, caryll_to_f2dot14(r->a));
			bufwrite16b(gbuf, caryll_to_f2dot14(r->b));
			bufwrite16b(gbuf, caryll_to_f2dot14(r->c));
			bufwrite16b(gbuf, caryll_to_f2dot14(r->d));
		}
	}
	if (g->instructionsLength) {
		bufwrite16b(gbuf, g->instructionsLength);
		if (g->instructions) bufwrite_bytes(gbuf, g->instructionsLength, g->instructions);
	}
}
void caryll_write_glyf(table_glyf *table, table_head *head, caryll_buffer *bufglyf,
                       caryll_buffer *bufloca) {
	caryll_buffer *gbuf = bufnew();

	uint32_t *loca = malloc((table->numberGlyphs + 1) * sizeof(uint32_t));
	for (uint16_t j = 0; j < table->numberGlyphs; j++) {
		loca[j] = bufglyf->cursor;
		glyf_glyph *g = table->glyphs[j];
		bufclear(gbuf);
		if (g->numberOfContours > 0) {
			glyf_write_simple(g, gbuf);
		} else if (g->numberOfReferences > 0) {
			glyf_write_composite(g, gbuf);
		}
		// pad extra zeroes
		buflongalign(gbuf);
		bufwrite_buf(bufglyf, gbuf);
	}
	loca[table->numberGlyphs] = bufglyf->cursor;
	if (bufglyf->cursor >= 0x20000) {
		head->indexToLocFormat = 1;
	} else {
		head->indexToLocFormat = 0;
	}
	// write loca table
	for (uint32_t j = 0; j <= table->numberGlyphs; j++) {
		if (head->indexToLocFormat) {
			bufwrite32b(bufloca, loca[j]);
		} else {
			bufwrite16b(bufloca, loca[j] >> 1);
		}
	}
	buffree(gbuf);
	free(loca);
}
