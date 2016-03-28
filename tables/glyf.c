#include "glyf.h"
#include <math.h>

glyf_glyph *caryll_glyf_new() {
	glyf_glyph *g = malloc(sizeof(glyf_glyph));
	g->numberOfContours = 0;
	g->numberOfReferences = 0;
	g->instructionsLength = 0;
	g->instructions = NULL;
	g->contours = NULL;
	g->references = NULL;
	g->advanceWidth = 0;
	g->name = NULL;
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

static INLINE glyf_glyph *caryll_read_simple_glyph(font_file_pointer start, uint16_t numberOfContours) {
	glyf_glyph *g = caryll_glyf_new();
	g->numberOfContours = numberOfContours;
	g->numberOfReferences = 0;

	glyf_contour *contours = (glyf_contour *)malloc(sizeof(glyf_contour) * numberOfContours);
	uint16_t lastPointIndex = 0;
	for (uint16_t j = 0; j < numberOfContours; j++) {
		uint16_t lastPointInCurrentContour = caryll_blt16u(start + 2 * j);
		contours[j].pointsCount = lastPointInCurrentContour - lastPointIndex + 1;
		contours[j].points = (glyf_point *)malloc(sizeof(glyf_point) * contours[j].pointsCount);
		lastPointIndex = lastPointInCurrentContour + 1;
	}

	uint16_t instructionLength = caryll_blt16u(start + 2 * numberOfContours);
	uint8_t *instructions = NULL;
	if (instructionLength > 0) {
		instructions = (font_file_pointer)malloc(sizeof(uint8_t) * instructionLength);
		memcpy(instructions, start + 2 * numberOfContours + 2, sizeof(uint8_t) * instructionLength);
	}
	g->instructionsLength = instructionLength;
	g->instructions = instructions;

	// read flags
	uint16_t pointsInGlyph = lastPointIndex;
	// There are repeating entries in the flags list, we will fill out the result
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
	uint8_t coordinatesOffset = 0;
	uint16_t coordinatesRead = 0;
	currentContour = 0;
	currentContourPointIndex = 0;
	while (coordinatesRead < pointsInGlyph) {
		uint8_t flag = flags[coordinatesRead];
		int16_t x;
		if (flag & GLYF_FLAG_X_SHORT) {
			x = (flag & GLYF_FLAG_POSITIVE_X ? 1 : -1) * caryll_blt8u(coordinatesStart + coordinatesOffset);
			coordinatesOffset += 1;
		} else {
			if (flag & GLYF_FLAG_SAME_X) {
				x = 0;
			} else {
				x = caryll_blt16s(coordinatesStart + coordinatesOffset);
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
			y = (flag & GLYF_FLAG_POSITIVE_Y ? 1 : -1) * caryll_blt8u(coordinatesStart + coordinatesOffset);
			coordinatesOffset += 1;
		} else {
			if (flag & GLYF_FLAG_SAME_Y) {
				y = 0;
			} else {
				y = caryll_blt16s(coordinatesStart + coordinatesOffset);
				coordinatesOffset += 2;
			}
		}
		next_point(contours, &currentContour, &currentContourPointIndex)->y = y;
		coordinatesRead += 1;
	}
	free(flags);
	// turn deltas to absolute coordiantes
	int16_t cx = 0;
	int16_t cy = 0;
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
	glyf_glyph *g = caryll_glyf_new();
	g->numberOfContours = 0;
	// pass 1, read references quantity
	uint16_t flags;
	uint16_t numberOfReferences = 0;
	uint32_t offset = 0;
	bool glyphHasInstruction = false;
	do {
		flags = caryll_blt16u(start + offset);
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
		flags = caryll_blt16u(start + offset);
		uint16_t index = caryll_blt16u(start + offset + 2);
		int16_t x = 0;
		int16_t y = 0;

		offset += 4; // flags & index
		if (flags & ARG_1_AND_2_ARE_WORDS) {
			x = caryll_blt16s(start + offset);
			y = caryll_blt16s(start + offset + 2);
			offset += 4;
		} else {
			x = caryll_blt8s(start + offset);
			y = caryll_blt8s(start + offset + 1);
			offset += 2;
		}
		float a = 1.0;
		float b = 0.0;
		float c = 0.0;
		float d = 1.0;
		if (flags & WE_HAVE_A_SCALE) {
			a = d = caryll_from_f2dot14(caryll_blt16s(start + offset));
			offset += 2;
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			a = caryll_from_f2dot14(caryll_blt16s(start + offset));
			d = caryll_from_f2dot14(caryll_blt16s(start + offset + 2));
			offset += 4;
		} else if (flags & WE_HAVE_A_TWO_BY_TWO) {
			a = caryll_from_f2dot14(caryll_blt16s(start + offset));
			b = caryll_from_f2dot14(caryll_blt16s(start + offset + 2));
			c = caryll_from_f2dot14(caryll_blt16s(start + offset + 4));
			d = caryll_from_f2dot14(caryll_blt16s(start + offset + 2));
			offset += 8;
		}
		g->references[j].glyph.gid = index;
		g->references[j].a = a;
		g->references[j].b = b;
		g->references[j].c = c;
		g->references[j].d = d;
		g->references[j].x = x;
		g->references[j].y = y;
		g->references[j].overlap = flags & OVERLAP_COMPOUND;
		g->references[j].useMyMetrics = flags & USE_MY_METRICS;
	}
	if (glyphHasInstruction) {
		uint16_t instructionLength = caryll_blt16u(start + offset);
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
	int16_t numberOfContours = caryll_blt16u(start);
	if (numberOfContours > 0) {
		return caryll_read_simple_glyph(start + 10, numberOfContours);
	} else {
		return caryll_read_composite_glyph(start + 10);
	}
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
				offsets[j] = caryll_blt32u(data + j * 4);
			} else {
				offsets[j] = caryll_blt16u(data + j * 2) * 2;
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

		glyf = (table_glyf *)malloc(sizeof(table_glyf *));
		glyf->numberGlyphs = numGlyphs;
		glyf->glyphs = malloc(sizeof(glyf_glyph) * numGlyphs);

		for (uint16_t j = 0; j < numGlyphs; j++) {
			if (offsets[j] < offsets[j + 1]) { // non-space glyph
				glyf->glyphs[j] = caryll_read_glyph(data, offsets[j]);
			} else { // space glyph
				glyf->glyphs[j] = caryll_glyf_new();
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

void caryll_delete_glyf(table_glyf *table) {
	if (table->glyphs) {
		for (uint16_t j = 0; j < table->numberGlyphs; j++) {
			glyf_glyph *g = table->glyphs[j];
			if (g) {
				if (g->numberOfContours > 0 && g->contours != NULL) {
					for (uint16_t k = 0; k < g->numberOfContours; k++) {
						if (g->contours[k].points) free(g->contours[k].points);
					}
					free(g->contours);
				}
				if (g->numberOfReferences > 0 && g->references != NULL) {
					for (uint16_t k = 0; k < g->numberOfReferences; k++) {
						g->references[k].glyph.name = NULL;
					}
					free(g->references);
				}
				if (g->instructions) { free(g->instructions); }
				g->name = NULL;
				free(g);
			}
		}
	}
	free(table->glyphs);
	free(table);
}

static INLINE json_value *coord_to_json(float z) {
	if (roundf(z) == z) {
		return json_integer_new(z);
	} else {
		return json_double_new(z);
	}
}
static INLINE json_value *glyf_glyph_contours_to_json(glyf_glyph *g, caryll_dump_options dumpopts) {
	json_value *contours = json_array_new(g->numberOfContours);
	for (uint16_t k = 0; k < g->numberOfContours; k++) {
		glyf_contour c = g->contours[k];
		json_value *contour = json_array_new(c.pointsCount);
		for (uint16_t m = 0; m < c.pointsCount; m++) {
			json_value *point = json_object_new(3);
			json_object_push(point, "x", coord_to_json(c.points[m].x));
			json_object_push(point, "y", coord_to_json(c.points[m].y));
			json_object_push(point, "on", json_boolean_new(c.points[m].onCurve));
			json_array_push(contour, point);
		}
		json_array_push(contours, contour);
	}
	return contours;
}
static INLINE json_value *glyf_glyph_references_to_json(glyf_glyph *g, caryll_dump_options dumpopts) {
	json_value *references = json_array_new(g->numberOfReferences);
	for (uint16_t k = 0; k < g->numberOfReferences; k++) {
		glyf_reference r = g->references[k];
		json_value *ref = json_object_new(9);
		json_object_push(ref, "glyph", json_string_new_length(sdslen(r.glyph.name), r.glyph.name));
		json_object_push(ref, "x", coord_to_json(r.x));
		json_object_push(ref, "y", coord_to_json(r.y));
		json_object_push(ref, "a", coord_to_json(r.a));
		json_object_push(ref, "b", coord_to_json(r.b));
		json_object_push(ref, "c", coord_to_json(r.c));
		json_object_push(ref, "d", coord_to_json(r.d));
		json_object_push(ref, "overlap", json_boolean_new(r.overlap));
		json_object_push(ref, "useMyMetrics", json_boolean_new(r.useMyMetrics));
		json_array_push(references, ref);
	}
	return references;
}
static INLINE json_value *glyf_glyph_instructions_to_json(glyf_glyph *g, caryll_dump_options dumpopts) {
	json_value *instructions = json_array_new(g->instructionsLength);
	for (uint16_t j = 0; j < g->instructionsLength; j++) {
		json_array_push(instructions, json_integer_new(g->instructions[j]));
	}
	return instructions;
}
static INLINE json_value *glyf_glyph_to_json(glyf_glyph *g, caryll_dump_options dumpopts) {
	json_value *glyph = json_object_new(4);
	json_object_push(glyph, "name", json_string_new_length(sdslen(g->name), g->name));
	json_object_push(glyph, "advanceWidth", json_integer_new(g->advanceWidth));
	json_object_push(glyph, "contours", glyf_glyph_contours_to_json(g, dumpopts));
	json_object_push(glyph, "references", glyf_glyph_references_to_json(g, dumpopts));
	if (!dumpopts.ignore_instructions) {
		json_object_push(glyph, "instructions", glyf_glyph_instructions_to_json(g, dumpopts));
	}
	return glyph;
}
void caryll_glyphorder_to_json(table_glyf *table, json_value *root) {
	if (!table) return;
	json_value *order = json_array_new(table->numberGlyphs);
	for (uint16_t j = 0; j < table->numberGlyphs; j++) {
		json_array_push(order, json_string_new_length(sdslen(table->glyphs[j]->name), table->glyphs[j]->name));
	}
	json_object_push(root, "glyph_order", order);
}
void caryll_glyf_to_json(table_glyf *table, json_value *root, caryll_dump_options dumpopts) {
	if (!table) return;
	json_value *glyf = json_object_new(table->numberGlyphs);
	for (uint16_t j = 0; j < table->numberGlyphs; j++) {
		glyf_glyph *g = table->glyphs[j];
		json_object_push(glyf, g->name, glyf_glyph_to_json(g, dumpopts));
	}
	json_object_push(root, "glyf", glyf);

	if (!dumpopts.ignore_glyph_order) caryll_glyphorder_to_json(table, root);
}

static INLINE void glyf_point_from_json(glyf_point *point, json_value *pointdump) {
	point->x = json_obj_getnum(pointdump, "x");
	point->y = json_obj_getnum(pointdump, "y");
	point->onCurve = json_obj_getbool(pointdump, "on");
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
		ref->overlap = json_obj_getbool_fallback(refdump, "overlap", true);
		ref->useMyMetrics = json_obj_getbool_fallback(refdump, "useMyMetrics", false);
	} else {
		// Invalid glyph references
		ref->glyph.name = NULL;
		ref->x = 0.0;
		ref->y = 0.0;
		ref->a = 1.0;
		ref->b = 0.0;
		ref->c = 0.0;
		ref->d = 1.0;
		ref->overlap = true;
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
static INLINE void glyf_instructions_from_json(json_value *col, glyf_glyph *g) {
	if (!col) {
		g->instructionsLength = 0;
		g->instructions = NULL;
		return;
	}
	g->instructionsLength = col->u.array.length;
	g->instructions = calloc(g->instructionsLength, sizeof(uint8_t));
	for (uint16_t j = 0; j < g->instructionsLength; j++) {
		json_value *byte = col->u.array.values[j];
		if (byte && byte->type == json_integer)
			g->instructions[j] = byte->u.integer;
		else if (byte && byte->type == json_double)
			g->instructions[j] = byte->u.dbl;
	}
}
static INLINE glyf_glyph *caryll_glyf_glyph_from_json(json_value *glyphdump, glyph_order_entry *order_entry) {
	glyf_glyph *g = malloc(sizeof(glyf_glyph));
	g->name = order_entry->name;
	g->advanceWidth = json_obj_getint(glyphdump, "advanceWidth");
	glyf_contours_from_json(json_obj_get_type(glyphdump, "contours", json_array), g);
	glyf_references_from_json(json_obj_get_type(glyphdump, "references", json_array), g);
	glyf_instructions_from_json(json_obj_get_type(glyphdump, "instructions", json_array), g);
	return g;
}

table_glyf *caryll_glyf_from_json(json_value *root, glyph_order_hash glyph_order) {
	if (root->type != json_object || !glyph_order) return NULL;
	table_glyf *glyf = NULL;
	json_value *table;
	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		uint16_t numGlyphs = table->u.object.length;
		glyf = malloc(sizeof(table_glyf));
		glyf->numberGlyphs = numGlyphs;
		glyf->glyphs = calloc(numGlyphs, sizeof(glyf_glyph *));
		for (uint16_t j = 0; j < numGlyphs; j++) {
			sds gname = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
			json_value *glyphdump = table->u.object.values[j].value;
			glyph_order_entry *order_entry;
			HASH_FIND_STR(glyph_order, gname, order_entry);
			if (glyphdump->type == json_object && order_entry && !glyf->glyphs[order_entry->gid]) {
				glyf->glyphs[order_entry->gid] = caryll_glyf_glyph_from_json(glyphdump, order_entry);
			}
			sdsfree(gname);
		}
		return glyf;
	}
	return NULL;
}

typedef enum { stat_not_started = 0, stat_doing = 1, stat_completed = 2 } stat_status;

glyf_glyph_stat stat_single_glyph(table_glyf *table, glyf_reference *gr, stat_status *stated, uint8_t depth,
                                  uint16_t topj) {
	glyf_glyph_stat stat = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint16_t j = gr->glyph.gid;
	if (depth >= 0xFF) return stat;
	if (stated[j] == stat_doing) {
		// We have a circular reference
		fprintf(stderr, "[Stat] Circular glyph reference found in gid %d to gid %d. The reference will be dropped.\n",
		        topj, j);
		stated[j] = stat_completed;
		return stat;
	}

	glyf_glyph *g = table->glyphs[gr->glyph.gid];
	stated[j] = stat_doing;
	float xmin = 0xFFFF;
	float xmax = -0xFFFF;
	float ymin = 0xFFFF;
	float ymax = -0xFFFF;
	uint16_t nestDepth = 0;
	uint16_t nPoints = 0;
	uint16_t nCompositePoints = 0;
	uint16_t nCompositeContours = 0;
	// Stat xmin, xmax, ymin, ymax
	for (uint16_t c = 0; c < g->numberOfContours; c++) {
		for (uint16_t pj = 0; pj < g->contours[c].pointsCount; pj++) {
			// Stat point coordinates USING the matrix transformation
			glyf_point *p = &(g->contours[c].points[pj]);
			float x = gr->x + gr->a * p->x + gr->b * p->y;
			float y = gr->y + gr->c * p->x + gr->d * p->y;
			if (x < xmin) xmin = x;
			if (x > xmax) xmax = x;
			if (y < ymin) ymin = y;
			if (y > ymax) ymax = y;
			nPoints += 1;
		}
	}
	nCompositePoints = nPoints;
	nCompositeContours = g->numberOfContours;
	for (uint16_t r = 0; r < g->numberOfReferences; r++) {
		glyf_reference ref;
		glyf_reference *rr = &(g->references[r]);
		ref.glyph.gid = g->references[r].glyph.gid;
		ref.glyph.name = NULL;
		// composite affine transformations
		ref.a = gr->a * rr->a + rr->b * gr->c;
		ref.b = rr->a * gr->b + rr->b * gr->d;
		ref.c = gr->a * rr->c + gr->c * rr->d;
		ref.d = gr->b * rr->c + rr->d * gr->d;
		ref.x = rr->x + rr->a * gr->x + rr->b * gr->y;
		ref.y = rr->y + rr->c * gr->x + rr->d * gr->y;

		glyf_glyph_stat thatstat = stat_single_glyph(table, &ref, stated, depth + 1, topj);
		if (thatstat.xMin < xmin) xmin = thatstat.xMin;
		if (thatstat.xMax > xmax) xmax = thatstat.xMax;
		if (thatstat.yMin < ymin) ymin = thatstat.yMin;
		if (thatstat.yMax > ymax) ymax = thatstat.yMax;
		if (thatstat.nestDepth + 1 > nestDepth) nestDepth = thatstat.nestDepth + 1;
		nCompositePoints += thatstat.nCompositePoints;
		nCompositeContours += thatstat.nCompositeContours;
	}
	stat.xMin = xmin;
	stat.xMax = xmax;
	stat.yMin = ymin;
	stat.yMax = ymax;
	stat.nestDepth = nestDepth;
	stat.nPoints = nPoints;
	stat.nContours = g->numberOfContours;
	stat.nCompositePoints = nCompositePoints;
	stat.nCompositeContours = nCompositeContours;
	stated[j] = stat_completed;
	return stat;
}

void caryll_stat_glyf(table_glyf *table) {
	stat_status *stated = calloc(table->numberGlyphs, sizeof(stat_status));
	for (uint16_t j = 0; j < table->numberGlyphs; j++) {
		glyf_reference gr;
		gr.glyph.gid = j;
		gr.glyph.name = NULL;
		gr.x = 0;
		gr.y = 0;
		gr.a = 1;
		gr.b = 0;
		gr.c = 0;
		gr.d = 1;
		table->glyphs[j]->stat = stat_single_glyph(table, &gr, stated, 0, j);
	}
	free(stated);
}

#define EPSILON (1e-5)

void caryll_write_glyf(table_glyf *table, caryll_buffer *bufglyf, caryll_buffer *bufloca) {
	caryll_buffer *gbuf = bufnew();
	caryll_buffer *flags = bufnew();
	caryll_buffer *xs = bufnew();
	caryll_buffer *ys = bufnew();
	for (uint16_t j = 0; j < table->numberGlyphs; j++) {
		glyf_glyph *g = table->glyphs[j];
		bufclear(gbuf);
		if (g->numberOfContours > 0) {
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
			int16_t cx = 0;
			int16_t cy = 0;
			for (uint16_t j = 0; j < g->numberOfContours; j++) {
				for (uint16_t k = 0; k < g->contours[j].pointsCount; k++) {
					glyf_point *p = &(g->contours[j].points[k]);
					uint8_t flag = p->onCurve ? GLYF_FLAG_ON_CURVE : 0;
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
					cx += dx;
					cy += dy;
				}
				bufwrite_buf(gbuf, flags);
				bufwrite_buf(gbuf, xs);
				bufwrite_buf(gbuf, ys);
			}
		} else if (g->numberOfReferences > 0) {
			bufwrite16b(gbuf, (-1));
			bufwrite16b(gbuf, (int16_t)g->stat.xMin);
			bufwrite16b(gbuf, (int16_t)g->stat.yMin);
			bufwrite16b(gbuf, (int16_t)g->stat.xMax);
			bufwrite16b(gbuf, (int16_t)g->stat.yMax);
			for (uint16_t j = 0; j < g->numberOfReferences; j++) {
				glyf_reference *r = &(g->references[j]);
				uint16_t flags = ARGS_ARE_XY_VALUES |
				                 (j < g->numberOfReferences - 1 ? MORE_COMPONENTS
				                                                : g->instructionsLength > 0 ? WE_HAVE_INSTRUCTIONS : 0);
				int16_t arg1 = r->x;
				int16_t arg2 = r->y;
				if (!(arg1 < 128 && arg1 >= -128 && arg2 < 128 && arg2 >= -128)) flags |= ARG_1_AND_2_ARE_WORDS;
				if (fabsf(r->b) > EPSILON || fabsf(r->c) > EPSILON) {
					flags |= WE_HAVE_A_TWO_BY_TWO;
				} else if (fabsf(r->a - 1) > EPSILON || fabsf(r->d - 1) > EPSILON) {
					if (fabsf(r->a - r->d) > EPSILON) {
						flags |= WE_HAVE_AN_X_AND_Y_SCALE;
					} else {
						flags |= WE_HAVE_A_SCALE;
					}
				}
				if (r->useMyMetrics) flags |= USE_MY_METRICS;

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
		bufwrite_buf(bufglyf, gbuf);
	}
	buffree(gbuf);
	buffree(flags);
	buffree(xs);
	buffree(ys);
}
