#include "support/util.h"
#include "support/ttinstr/ttinstr.h"
#include "otfcc/table/glyf.h"

typedef enum {
	GLYF_FLAG_ON_CURVE = 1,
	GLYF_FLAG_X_SHORT = (1 << 1),
	GLYF_FLAG_Y_SHORT = (1 << 2),
	GLYF_FLAG_REPEAT = (1 << 3),
	GLYF_FLAG_SAME_X = (1 << 4),
	GLYF_FLAG_SAME_Y = (1 << 5),
	GLYF_FLAG_POSITIVE_X = (1 << 4),
	GLYF_FLAG_POSITIVE_Y = (1 << 5)
} glyf_point_flag;

typedef enum {
	ARG_1_AND_2_ARE_WORDS = (1 << 0),
	ARGS_ARE_XY_VALUES = (1 << 1),
	ROUND_XY_TO_GRID = (1 << 2),
	WE_HAVE_A_SCALE = (1 << 3),
	MORE_COMPONENTS = (1 << 5),
	WE_HAVE_AN_X_AND_Y_SCALE = (1 << 6),
	WE_HAVE_A_TWO_BY_TWO = (1 << 7),
	WE_HAVE_INSTRUCTIONS = (1 << 8),
	USE_MY_METRICS = (1 << 9),
	OVERLAP_COMPOUND = (1 << 10)
} glyf_reference_flag;

typedef enum { MASK_ON_CURVE = 1 } glyf_oncurve_mask;

glyf_Glyph *otfcc_newGlyf_glyph() {
	glyf_Glyph *g;
	NEW(g);
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
	g->fdSelect = Handle.empty();
	g->yPel = 0;

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

static void otfcc_deleteGlyf_glyph(glyf_Glyph *g) {
	if (!g) return;
	sdsfree(g->name);
	if (g->numberOfContours > 0 && g->contours != NULL) {
		for (shapeid_t k = 0; k < g->numberOfContours; k++) {
			if (g->contours[k].points) FREE(g->contours[k].points);
		}
		FREE(g->contours);
	}
	if (g->numberOfReferences > 0 && g->references != NULL) {
		for (shapeid_t k = 0; k < g->numberOfReferences; k++) {
			Handle.dispose(&g->references[k].glyph);
		}
		FREE(g->references);
	}
	if (g->instructions) { FREE(g->instructions); }
	if (g->stemH) FREE(g->stemH);
	if (g->stemV) FREE(g->stemV);
	if (g->hintMasks) FREE(g->hintMasks);
	if (g->contourMasks) FREE(g->contourMasks);
	Handle.dispose(&g->fdSelect);
	g->name = NULL;
	FREE(g);
}

void otfcc_deleteGlyf(table_glyf *table) {
	if (table->glyphs) {
		for (glyphid_t j = 0; j < table->numberGlyphs; j++) {
			otfcc_deleteGlyf_glyph(table->glyphs[j]);
		}
		FREE(table->glyphs);
	}
	FREE(table);
}

static glyf_Point *next_point(glyf_Contour *contours, shapeid_t *cc, shapeid_t *cp) {
	if (*cp >= contours[*cc].pointsCount) {
		*cp = 0;
		*cc += 1;
	}
	return &contours[*cc].points[(*cp)++];
}

static glyf_Glyph *otfcc_read_simple_glyph(font_file_pointer start, shapeid_t numberOfContours) {
	glyf_Glyph *g = otfcc_newGlyf_glyph();
	g->numberOfContours = numberOfContours;
	g->numberOfReferences = 0;

	glyf_Contour *contours;
	NEW(contours, numberOfContours);
	shapeid_t lastPointIndex = 0;
	for (shapeid_t j = 0; j < numberOfContours; j++) {
		shapeid_t lastPointInCurrentContour = read_16u(start + 2 * j);
		contours[j].pointsCount = lastPointInCurrentContour - lastPointIndex + 1;
		NEW(contours[j].points, contours[j].pointsCount);
		lastPointIndex = lastPointInCurrentContour + 1;
	}

	uint16_t instructionLength = read_16u(start + 2 * numberOfContours);
	uint8_t *instructions = NULL;
	if (instructionLength > 0) {
		NEW(instructions, instructionLength);
		memcpy(instructions, start + 2 * numberOfContours + 2, sizeof(uint8_t) * instructionLength);
	}
	g->instructionsLength = instructionLength;
	g->instructions = instructions;

	// read flags
	shapeid_t pointsInGlyph = lastPointIndex;
	// There are repeating entries in the flags list, we will fill out the
	// result
	font_file_pointer flags;
	NEW(flags, pointsInGlyph);
	font_file_pointer flagStart = start + 2 * numberOfContours + 2 + instructionLength;
	shapeid_t flagsReadSofar = 0;
	shapeid_t flagBytesReadSofar = 0;

	shapeid_t currentContour = 0;
	shapeid_t currentContourPointIndex = 0;
	while (flagsReadSofar < pointsInGlyph) {
		uint8_t flag = flagStart[flagBytesReadSofar];
		flags[flagsReadSofar] = flag;
		flagBytesReadSofar += 1;
		flagsReadSofar += 1;
		next_point(contours, &currentContour, &currentContourPointIndex)->onCurve = (flag & GLYF_FLAG_ON_CURVE);
		if (flag & GLYF_FLAG_REPEAT) { // repeating flag
			uint8_t repeat = flagStart[flagBytesReadSofar];
			flagBytesReadSofar += 1;
			for (uint8_t j = 0; j < repeat; j++) {
				flags[flagsReadSofar + j] = flag;
				next_point(contours, &currentContour, &currentContourPointIndex)->onCurve = (flag & GLYF_FLAG_ON_CURVE);
			}
			flagsReadSofar += repeat;
		}
	}

	// read X coordinates
	font_file_pointer coordinatesStart = flagStart + flagBytesReadSofar;
	uint32_t coordinatesOffset = 0;
	shapeid_t coordinatesRead = 0;
	currentContour = 0;
	currentContourPointIndex = 0;
	while (coordinatesRead < pointsInGlyph) {
		uint8_t flag = flags[coordinatesRead];
		int16_t x;
		if (flag & GLYF_FLAG_X_SHORT) {
			x = (flag & GLYF_FLAG_POSITIVE_X ? 1 : -1) * read_8u(coordinatesStart + coordinatesOffset);
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
			y = (flag & GLYF_FLAG_POSITIVE_Y ? 1 : -1) * read_8u(coordinatesStart + coordinatesOffset);
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
	FREE(flags);
	// turn deltas to absolute coordiantes
	double cx = 0;
	double cy = 0;
	for (shapeid_t j = 0; j < numberOfContours; j++) {
		for (shapeid_t k = 0; k < contours[j].pointsCount; k++) {
			cx += contours[j].points[k].x;
			contours[j].points[k].x = cx;
			cy += contours[j].points[k].y;
			contours[j].points[k].y = cy;
		}
	}
	g->contours = contours;
	return g;
}

static glyf_Glyph *otfcc_read_composite_glyph(font_file_pointer start) {
	glyf_Glyph *g = otfcc_newGlyf_glyph();
	g->numberOfContours = 0;
	// pass 1, read references quantity
	uint16_t flags;
	shapeid_t numberOfReferences = 0;
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
	NEW(g->references, numberOfReferences);
	offset = 0;
	for (shapeid_t j = 0; j < numberOfReferences; j++) {
		flags = read_16u(start + offset);
		glyphid_t index = read_16u(start + offset + 2);
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
		double a = 1.0;
		double b = 0.0;
		double c = 0.0;
		double d = 1.0;
		if (flags & WE_HAVE_A_SCALE) {
			a = d = otfcc_from_f2dot14(read_16s(start + offset));
			offset += 2;
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			a = otfcc_from_f2dot14(read_16s(start + offset));
			d = otfcc_from_f2dot14(read_16s(start + offset + 2));
			offset += 4;
		} else if (flags & WE_HAVE_A_TWO_BY_TWO) {
			a = otfcc_from_f2dot14(read_16s(start + offset));
			b = otfcc_from_f2dot14(read_16s(start + offset + 2));
			c = otfcc_from_f2dot14(read_16s(start + offset + 4));
			d = otfcc_from_f2dot14(read_16s(start + offset + 2));
			offset += 8;
		}
		g->references[j].glyph = Handle.fromIndex(index);
		g->references[j].a = a;
		g->references[j].b = b;
		g->references[j].c = c;
		g->references[j].d = d;
		g->references[j].x = (double)x;
		g->references[j].y = (double)y;
		g->references[j].roundToGrid = !!(flags & ROUND_XY_TO_GRID);
		g->references[j].useMyMetrics = !!(flags & USE_MY_METRICS);
	}
	if (glyphHasInstruction) {
		uint16_t instructionLength = read_16u(start + offset);
		font_file_pointer instructions = NULL;
		if (instructionLength > 0) {
			NEW(instructions, instructionLength);
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

static glyf_Glyph *otfcc_read_glyph(font_file_pointer data, uint32_t offset) {
	font_file_pointer start = data + offset;
	int16_t numberOfContours = read_16u(start);
	glyf_Glyph *g;
	if (numberOfContours > 0) {
		g = otfcc_read_simple_glyph(start + 10, numberOfContours);
	} else {
		g = otfcc_read_composite_glyph(start + 10);
	}
	g->stat.xMin = read_16s(start + 2);
	g->stat.yMin = read_16s(start + 4);
	g->stat.xMax = read_16s(start + 6);
	g->stat.yMax = read_16s(start + 8);
	return g;
}

table_glyf *otfcc_readGlyf(const otfcc_Packet packet, const otfcc_Options *options, table_head *head,
                           table_maxp *maxp) {
	if (head == NULL || maxp == NULL) return NULL;
	uint32_t *offsets = NULL;
	table_glyf *glyf = NULL;

	uint16_t locaIsLong = head->indexToLocFormat;
	glyphid_t numGlyphs = maxp->numGlyphs;
	NEW(offsets, (numGlyphs + 1));
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
		logWarning("table 'loca' corrupted.\n");
		if (offsets) { FREE(offsets), offsets = NULL; }
		continue;
	}
	if (!foundLoca) goto ABSENT;

	// read glyf
	FOR_TABLE('glyf', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < offsets[numGlyphs]) goto GLYF_CORRUPTED;

		NEW(glyf);
		glyf->numberGlyphs = numGlyphs;
		NEW(glyf->glyphs, numGlyphs);

		for (glyphid_t j = 0; j < numGlyphs; j++) {
			if (offsets[j] < offsets[j + 1]) { // non-space glyph
				glyf->glyphs[j] = otfcc_read_glyph(data, offsets[j]);
			} else { // space glyph
				glyf->glyphs[j] = otfcc_newGlyf_glyph();
			}
		}
		goto PRESENT;
	GLYF_CORRUPTED:
		logWarning("table 'glyf' corrupted.\n");
		if (glyf) { otfcc_deleteGlyf(glyf), glyf = NULL; }
	}
	goto ABSENT;

PRESENT:
	if (offsets) { FREE(offsets), offsets = NULL; }
	return glyf;

ABSENT:
	if (offsets) { FREE(offsets), offsets = NULL; }
	if (glyf) { FREE(glyf), glyf = NULL; }
	return NULL;
}

// to json
static void glyf_glyph_dump_contours(glyf_Glyph *g, json_value *target) {
	if (!g->numberOfContours || !g->contours) return;
	json_value *contours = json_array_new(g->numberOfContours);
	for (shapeid_t k = 0; k < g->numberOfContours; k++) {
		glyf_Contour *c = &(g->contours[k]);
		json_value *contour = json_array_new(c->pointsCount);
		for (shapeid_t m = 0; m < c->pointsCount; m++) {
			json_value *point = json_object_new(4);
			json_object_push(point, "x", json_new_position(c->points[m].x));
			json_object_push(point, "y", json_new_position(c->points[m].y));
			json_object_push(point, "on", json_boolean_new(c->points[m].onCurve & MASK_ON_CURVE));
			json_array_push(contour, point);
		}
		json_array_push(contours, contour);
	}
	json_object_push(target, "contours", preserialize(contours));
}
static void glyf_glyph_dump_references(glyf_Glyph *g, json_value *target) {
	if (!g->numberOfReferences || !g->references) return;
	json_value *references = json_array_new(g->numberOfReferences);
	for (shapeid_t k = 0; k < g->numberOfReferences; k++) {
		glyf_ComponentReference *r = &(g->references[k]);
		json_value *ref = json_object_new(9);
		json_object_push(ref, "glyph", json_string_new_length((uint32_t)sdslen(r->glyph.name), r->glyph.name));
		json_object_push(ref, "x", json_new_position(r->x));
		json_object_push(ref, "y", json_new_position(r->y));
		json_object_push(ref, "a", json_new_position(r->a));
		json_object_push(ref, "b", json_new_position(r->b));
		json_object_push(ref, "c", json_new_position(r->c));
		json_object_push(ref, "d", json_new_position(r->d));
		if (r->roundToGrid) { json_object_push(ref, "roundToGrid", json_boolean_new(true)); }
		if (r->useMyMetrics) { json_object_push(ref, "useMyMetrics", json_boolean_new(true)); }
		json_array_push(references, ref);
	}
	json_object_push(target, "references", preserialize(references));
}
static json_value *glyf_glyph_dump_stemdefs(glyf_PostscriptStemDef *stems, shapeid_t count) {
	json_value *a = json_array_new(count);
	for (shapeid_t j = 0; j < count; j++) {
		json_value *stem = json_object_new(3);
		json_object_push(stem, "position", json_new_position(stems[j].position));
		json_object_push(stem, "width", json_new_position(stems[j].width));
		json_array_push(a, stem);
	}
	return a;
}
static json_value *glyf_glyph_dump_maskdefs(glyf_PostscriptHintMask *masks, shapeid_t count, shapeid_t nh,
                                            shapeid_t nv) {
	json_value *a = json_array_new(count);
	for (shapeid_t j = 0; j < count; j++) {
		json_value *mask = json_object_new(3);
		json_object_push(mask, "pointsBefore", json_integer_new(masks[j].pointsBefore));
		json_value *h = json_array_new(nh);
		for (shapeid_t k = 0; k < nh; k++) {
			json_array_push(h, json_boolean_new(masks[j].maskH[k]));
		}
		json_object_push(mask, "maskH", h);
		json_value *v = json_array_new(nv);
		for (shapeid_t k = 0; k < nv; k++) {
			json_array_push(v, json_boolean_new(masks[j].maskV[k]));
		}
		json_object_push(mask, "maskV", v);
		json_array_push(a, mask);
	}
	return a;
}

static json_value *glyf_dump_glyph(glyf_Glyph *g, const otfcc_Options *options, bool hasVerticalMetrics,
                                   bool exportFDSelect) {
	json_value *glyph = json_object_new(12);
	json_object_push(glyph, "advanceWidth", json_new_position(g->advanceWidth));
	if (hasVerticalMetrics) {
		json_object_push(glyph, "advanceHeight", json_new_position(g->advanceHeight));
		json_object_push(glyph, "verticalOrigin", json_new_position(g->verticalOrigin));
	}
	glyf_glyph_dump_contours(g, glyph);
	glyf_glyph_dump_references(g, glyph);
	if (!options->ignore_hints && g->instructions && g->instructionsLength) {
		json_object_push(glyph, "instructions", dump_ttinstr(g->instructions, g->instructionsLength, options));
	}
	if (!options->ignore_hints && g->stemH && g->numberOfStemH) {
		json_object_push(glyph, "stemH", preserialize(glyf_glyph_dump_stemdefs(g->stemH, g->numberOfStemH)));
	}
	if (!options->ignore_hints && g->stemV && g->numberOfStemV) {
		json_object_push(glyph, "stemV", preserialize(glyf_glyph_dump_stemdefs(g->stemV, g->numberOfStemV)));
	}
	if (!options->ignore_hints && g->hintMasks && g->numberOfHintMasks) {
		json_object_push(glyph, "hintMasks",
		                 preserialize(glyf_glyph_dump_maskdefs(g->hintMasks, g->numberOfHintMasks, g->numberOfStemH,
		                                                       g->numberOfStemV)));
	}
	if (!options->ignore_hints && g->contourMasks && g->numberOfContourMasks) {
		json_object_push(glyph, "contourMasks",
		                 preserialize(glyf_glyph_dump_maskdefs(g->contourMasks, g->numberOfContourMasks,
		                                                       g->numberOfStemH, g->numberOfStemV)));
	}
	if (exportFDSelect) { json_object_push(glyph, "CFF_fdSelect", json_string_new(g->fdSelect.name)); }
	if (g->yPel) { json_object_push(glyph, "LTSH_yPel", json_integer_new(g->yPel)); }
	return glyph;
}
void otfcc_dump_glyphorder(const table_glyf *table, json_value *root) {
	if (!table) return;
	json_value *order = json_array_new(table->numberGlyphs);
	for (glyphid_t j = 0; j < table->numberGlyphs; j++) {
		json_array_push(order,
		                json_string_new_length((uint32_t)sdslen(table->glyphs[j]->name), table->glyphs[j]->name));
	}
	json_object_push(root, "glyph_order", preserialize(order));
}
void otfcc_dumpGlyf(const table_glyf *table, json_value *root, const otfcc_Options *options, bool hasVerticalMetrics,
                    bool exportFDSelect) {
	if (!table) return;
	loggedStep("glyf") {
		json_value *glyf = json_object_new(table->numberGlyphs);
		for (glyphid_t j = 0; j < table->numberGlyphs; j++) {
			glyf_Glyph *g = table->glyphs[j];
			json_object_push(glyf, g->name, glyf_dump_glyph(g, options, hasVerticalMetrics, exportFDSelect));
		}
		json_object_push(root, "glyf", glyf);
		if (!options->ignore_glyph_order) otfcc_dump_glyphorder(table, root);
	}
}

// from json
static void glyf_parse_point(glyf_Point *point, json_value *pointdump) {
	point->x = 0;
	point->y = 0;
	point->onCurve = 0;
	if (!pointdump || pointdump->type != json_object) return;
	for (uint32_t _k = 0; _k < pointdump->u.object.length; _k++) {
		char *ck = pointdump->u.object.values[_k].name;
		json_value *cv = pointdump->u.object.values[_k].value;
		if (strcmp(ck, "x") == 0) {
			point->x = json_numof(cv);
		} else if (strcmp(ck, "y") == 0) {
			point->y = json_numof(cv);
		} else if (strcmp(ck, "on") == 0) {
			point->onCurve = json_boolof(cv);
		}
	}
}
static void glyf_parse_reference(glyf_ComponentReference *ref, json_value *refdump) {
	json_value *_gname = json_obj_get_type(refdump, "glyph", json_string);
	if (_gname) {
		ref->glyph = Handle.fromName(sdsnewlen(_gname->u.string.ptr, _gname->u.string.length));
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
static void glyf_parse_contours(json_value *col, glyf_Glyph *g) {
	if (!col) {
		g->numberOfContours = 0;
		g->contours = NULL;
		return;
	}
	g->numberOfContours = col->u.array.length;
	NEW(g->contours, g->numberOfContours);
	for (shapeid_t j = 0; j < g->numberOfContours; j++) {
		json_value *contourdump = col->u.array.values[j];
		if (contourdump && contourdump->type == json_array) {
			g->contours[j].pointsCount = contourdump->u.array.length;
			NEW(g->contours[j].points, g->contours[j].pointsCount);
			for (shapeid_t k = 0; k < g->contours[j].pointsCount; k++) {
				glyf_parse_point(&(g->contours[j].points[k]), contourdump->u.array.values[k]);
			}
		} else {
			g->contours[j].pointsCount = 0;
			g->contours[j].points = NULL;
		}
	}
}
static void glyf_parse_references(json_value *col, glyf_Glyph *g) {
	if (!col) {
		g->numberOfReferences = 0;
		g->references = NULL;
		return;
	}
	g->numberOfReferences = col->u.array.length;
	NEW(g->references, g->numberOfReferences);
	for (shapeid_t j = 0; j < g->numberOfReferences; j++) {
		glyf_parse_reference(&(g->references[j]), col->u.array.values[j]);
	}
}

static void makeInstrsForGlyph(void *_g, uint8_t *instrs, uint32_t len) {
	glyf_Glyph *g = (glyf_Glyph *)_g;
	g->instructionsLength = len;
	g->instructions = instrs;
}
static void wrongInstrsForGlyph(void *_g, char *reason, int pos) {
	/*
	glyf_Glyph *g = (glyf_Glyph *)_g;
	fprintf(stderr, "[OTFCC] TrueType instructions parse error : %s, at %d in /%s\n", reason, pos, g->name);
	*/
}

static void parse_stems(json_value *sd, shapeid_t *count, glyf_PostscriptStemDef **arr) {
	if (!sd) {
		*arr = NULL;
		*count = 0;
	} else {
		*count = sd->u.array.length;
		NEW(*arr, *count);
		shapeid_t jj = 0;
		for (shapeid_t j = 0; j < *count; j++) {
			json_value *s = sd->u.array.values[j];
			if (s->type == json_object) {
				(*arr)[jj].position = json_obj_getnum(s, "position");
				(*arr)[jj].width = json_obj_getnum(s, "width");
				jj++;
			}
		}
		*count = jj;
	}
}
static void parse_maskbits(bool *arr, json_value *bits) {
	if (!bits) {
		for (shapeid_t j = 0; j < 0x100; j++) {
			arr[j] = false;
		}
	} else {
		for (shapeid_t j = 0; j < 0x100 && j < bits->u.array.length; j++) {
			json_value *b = bits->u.array.values[j];
			switch (b->type) {
				case json_boolean:
					arr[j] = b->u.boolean;
					break;
				case json_integer:
					arr[j] = b->u.integer;
					break;
				case json_double:
					arr[j] = b->u.dbl;
					break;
				default:
					arr[j] = false;
			}
		}
	}
}
static void parse_masks(json_value *md, shapeid_t *count, glyf_PostscriptHintMask **arr) {
	if (!md) {
		*arr = NULL;
		*count = 0;
	} else {
		*count = md->u.array.length;
		NEW(*arr, *count);
		shapeid_t jj = 0;
		for (shapeid_t j = 0; j < *count; j++) {
			json_value *m = md->u.array.values[j];
			if (m->type == json_object) {
				(*arr)[jj].pointsBefore = json_obj_getint(m, "pointsBefore");
				parse_maskbits(&((*arr)[jj].maskH[0]), json_obj_get_type(m, "maskH", json_array));
				parse_maskbits(&((*arr)[jj].maskV[0]), json_obj_get_type(m, "maskV", json_array));
				jj++;
			}
		}
		*count = jj;
	}
}

static glyf_Glyph *otfcc_glyf_parse_glyph(json_value *glyphdump, otfcc_GlyphOrderEntry *order_entry,
                                          const otfcc_Options *options) {
	glyf_Glyph *g = otfcc_newGlyf_glyph();
	g->name = sdsdup(order_entry->name);
	g->advanceWidth = json_obj_getint(glyphdump, "advanceWidth");
	g->advanceHeight = json_obj_getint(glyphdump, "advanceHeight");
	g->verticalOrigin = json_obj_getint(glyphdump, "verticalOrigin");
	glyf_parse_contours(json_obj_get_type(glyphdump, "contours", json_array), g);
	glyf_parse_references(json_obj_get_type(glyphdump, "references", json_array), g);
	if (!options->ignore_hints) {
		parse_ttinstr(json_obj_get(glyphdump, "instructions"), g, makeInstrsForGlyph, wrongInstrsForGlyph);
		parse_stems(json_obj_get_type(glyphdump, "stemH", json_array), &g->numberOfStemH, &(g->stemH));
		parse_stems(json_obj_get_type(glyphdump, "stemV", json_array), &g->numberOfStemV, &(g->stemV));
		parse_masks(json_obj_get_type(glyphdump, "hintMasks", json_array), &(g->numberOfHintMasks), &(g->hintMasks));
		parse_masks(json_obj_get_type(glyphdump, "contourMasks", json_array), &(g->numberOfContourMasks),
		            &(g->contourMasks));
	}
	// Glyph data of other tables
	g->fdSelect = Handle.fromName(json_obj_getsds(glyphdump, "CFF_fdSelect"));
	g->yPel = json_obj_getint(glyphdump, "LTSH_yPel");
	if (!g->yPel) { g->yPel = json_obj_getint(glyphdump, "yPel"); }
	return g;
}

table_glyf *otfcc_parseGlyf(json_value *root, otfcc_GlyphOrder *glyph_order, const otfcc_Options *options) {
	if (root->type != json_object || !glyph_order) return NULL;
	table_glyf *glyf = NULL;
	json_value *table;
	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		loggedStep("glyf") {
			glyphid_t numGlyphs = table->u.object.length;
			NEW(glyf);
			glyf->numberGlyphs = numGlyphs;
			NEW(glyf->glyphs, numGlyphs);
			for (glyphid_t j = 0; j < numGlyphs; j++) {
				sds gname = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
				json_value *glyphdump = table->u.object.values[j].value;
				otfcc_GlyphOrderEntry *order_entry = NULL;
				HASH_FIND(hhName, glyph_order->byName, gname, sdslen(gname), order_entry);
				if (glyphdump->type == json_object && order_entry && !glyf->glyphs[order_entry->gid]) {
					glyf->glyphs[order_entry->gid] = otfcc_glyf_parse_glyph(glyphdump, order_entry, options);
				}
				json_value_free(glyphdump);
				table->u.object.values[j].value = NULL;
				sdsfree(gname);
			}
		}
		return glyf;
	}
	return NULL;
}

caryll_Buffer *shrinkFlags(caryll_Buffer *flags) {
	if (!buflen(flags)) return (flags);
	caryll_Buffer *shrunk = bufnew();
	bufwrite8(shrunk, flags->data[0]);
	int repeating = 0;
	for (size_t j = 1; j < buflen(flags); j++) {
		if (flags->data[j] == flags->data[j - 1]) {
			if (repeating && repeating < 0xFE) {
				shrunk->data[shrunk->cursor - 1] += 1;
				repeating += 1;
			} else if (repeating == 0) {
				shrunk->data[shrunk->cursor - 1] |= GLYF_FLAG_REPEAT;
				bufwrite8(shrunk, 1);
				repeating += 1;
			} else {
				repeating = 0;
				bufwrite8(shrunk, flags->data[j]);
			}
		} else {
			repeating = 0;
			bufwrite8(shrunk, flags->data[j]);
		}
	}
	buffree(flags);
	return shrunk;
}

// serialize
#define EPSILON (1e-5)
static void glyf_build_simple(const glyf_Glyph *g, caryll_Buffer *gbuf) {
	caryll_Buffer *flags = bufnew();
	caryll_Buffer *xs = bufnew();
	caryll_Buffer *ys = bufnew();

	bufwrite16b(gbuf, g->numberOfContours);
	bufwrite16b(gbuf, (int16_t)g->stat.xMin);
	bufwrite16b(gbuf, (int16_t)g->stat.yMin);
	bufwrite16b(gbuf, (int16_t)g->stat.xMax);
	bufwrite16b(gbuf, (int16_t)g->stat.yMax);

	// endPtsOfContours[n]
	shapeid_t ptid = 0;
	for (shapeid_t j = 0; j < g->numberOfContours; j++) {
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
	int32_t cx = 0;
	int32_t cy = 0;
	for (shapeid_t cj = 0; cj < g->numberOfContours; cj++) {
		for (shapeid_t k = 0; k < g->contours[cj].pointsCount; k++) {
			glyf_Point *p = &(g->contours[cj].points[k]);
			uint8_t flag = (p->onCurve & MASK_ON_CURVE) ? GLYF_FLAG_ON_CURVE : 0;
			int32_t px = round(p->x);
			int32_t py = round(p->y);
			int16_t dx = (int16_t)(px - cx);
			int16_t dy = (int16_t)(py - cy);
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
			cx = px;
			cy = py;
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
static void glyf_build_composite(const glyf_Glyph *g, caryll_Buffer *gbuf) {
	bufwrite16b(gbuf, (-1));
	bufwrite16b(gbuf, (int16_t)g->stat.xMin);
	bufwrite16b(gbuf, (int16_t)g->stat.yMin);
	bufwrite16b(gbuf, (int16_t)g->stat.xMax);
	bufwrite16b(gbuf, (int16_t)g->stat.yMax);
	for (shapeid_t rj = 0; rj < g->numberOfReferences; rj++) {
		glyf_ComponentReference *r = &(g->references[rj]);
		uint16_t flags =
		    ARGS_ARE_XY_VALUES |
		    (rj < g->numberOfReferences - 1 ? MORE_COMPONENTS : g->instructionsLength > 0 ? WE_HAVE_INSTRUCTIONS : 0);
		int16_t arg1 = r->x;
		int16_t arg2 = r->y;
		if (!(arg1 < 128 && arg1 >= -128 && arg2 < 128 && arg2 >= -128)) flags |= ARG_1_AND_2_ARE_WORDS;
		if (fabs(r->b) > EPSILON || fabs(r->c) > EPSILON) {
			flags |= WE_HAVE_A_TWO_BY_TWO;
		} else if (fabs(r->a - 1) > EPSILON || fabs(r->d - 1) > EPSILON) {
			if (fabs(r->a - r->d) > EPSILON) {
				flags |= WE_HAVE_AN_X_AND_Y_SCALE;
			} else {
				flags |= WE_HAVE_A_SCALE;
			}
		}
		if (r->roundToGrid) flags |= ROUND_XY_TO_GRID;
		if (r->useMyMetrics) flags |= USE_MY_METRICS;
		bufwrite16b(gbuf, flags);
		bufwrite16b(gbuf, r->glyph.index);
		if (flags & ARG_1_AND_2_ARE_WORDS) {
			bufwrite16b(gbuf, arg1);
			bufwrite16b(gbuf, arg2);
		} else {
			bufwrite8(gbuf, arg1);
			bufwrite8(gbuf, arg2);
		}
		if (flags & WE_HAVE_A_SCALE) {
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->a));
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->a));
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->d));
		} else if (flags & WE_HAVE_A_TWO_BY_TWO) {
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->a));
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->b));
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->c));
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->d));
		}
	}
	if (g->instructionsLength) {
		bufwrite16b(gbuf, g->instructionsLength);
		if (g->instructions) bufwrite_bytes(gbuf, g->instructionsLength, g->instructions);
	}
}
table_GlyfAndLocaBuffers otfcc_buildGlyf(const table_glyf *table, table_head *head, const otfcc_Options *options) {
	caryll_Buffer *bufglyf = bufnew();
	caryll_Buffer *bufloca = bufnew();
	if (table && head) {
		caryll_Buffer *gbuf = bufnew();
		uint32_t *loca;
		NEW(loca, table->numberGlyphs + 1);
		for (glyphid_t j = 0; j < table->numberGlyphs; j++) {
			loca[j] = (uint32_t)bufglyf->cursor;
			glyf_Glyph *g = table->glyphs[j];
			bufclear(gbuf);
			if (g->numberOfContours > 0) {
				glyf_build_simple(g, gbuf);
			} else if (g->numberOfReferences > 0) {
				glyf_build_composite(g, gbuf);
			}
			// pad extra zeroes
			buflongalign(gbuf);
			bufwrite_buf(bufglyf, gbuf);
		}
		loca[table->numberGlyphs] = (uint32_t)bufglyf->cursor;
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
		FREE(loca);
	}
	table_GlyfAndLocaBuffers pair = {bufglyf, bufloca};
	return pair;
}
