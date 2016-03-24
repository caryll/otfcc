#include "glyf.h"
#include <math.h>

glyf_glyph *spaceGlyph() {
	glyf_glyph *g = malloc(sizeof(glyf_glyph));
	g->numberOfContours = 0;
	g->numberOfReferences = 0;
	g->instructionsLength = 0;
	g->instructions = NULL;
	g->contours = NULL;
	g->references = NULL;
	g->advanceWidth = 0;
	g->name = NULL;
	return g;
}

static INLINE glyf_point *next_point(glyf_contour *contours, uint16_t *cc, uint16_t *cp) {
	if (*cp >= contours[*cc].pointsCount) {
		*cp = 0;
		*cc += 1;
	}
	return &contours[*cc].points[(*cp)++];
}

static glyf_glyph *caryll_read_simple_glyph(font_file_pointer start, uint16_t numberOfContours) {
	glyf_glyph *g = spaceGlyph();
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
			x = (flag & GLYF_FLAG_POSITIVE_X ? 1 : -1) * coordinatesStart[coordinatesOffset];
			coordinatesOffset += 1;
		} else {
			if (flag & GLYF_FLAG_SAME_X) {
				x = 0;
			} else {
				x = caryll_blt16u(coordinatesStart + coordinatesOffset);
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
			y = (flag & GLYF_FLAG_POSITIVE_Y ? 1 : -1) * coordinatesStart[coordinatesOffset];
			coordinatesOffset += 1;
		} else {
			if (flag & GLYF_FLAG_SAME_Y) {
				y = 0;
			} else {
				y = caryll_blt16u(coordinatesStart + coordinatesOffset);
				coordinatesOffset += 2;
			}
		}
		next_point(contours, &currentContour, &currentContourPointIndex)->y = y;
		coordinatesRead += 1;
	}
	free(flags);
	// turn deltas to absolute coordiantes
	for (uint16_t j = 0; j < numberOfContours; j++) {
		int16_t cx = 0;
		int16_t cy = 0;
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

static glyf_glyph *caryll_read_composite_glyph(font_file_pointer start) {
	glyf_glyph *g = spaceGlyph();
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
			x = caryll_blt16u(start + offset);
			y = caryll_blt16u(start + offset + 2);
			offset += 4;
		} else {
			x = caryll_blt16u(start + offset);
			y = caryll_blt16u(start + offset + 1);
			offset += 2;
		}
		float a = 1.0;
		float b = 0.0;
		float c = 0.0;
		float d = 1.0;
		if (flags & WE_HAVE_A_SCALE) {
			a = d = caryll_from_f2dot14(caryll_blt16u(start + offset));
			offset += 2;
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			a = caryll_from_f2dot14(caryll_blt16u(start + offset));
			d = caryll_from_f2dot14(caryll_blt16u(start + offset + 2));
			offset += 4;
		} else if (flags & WE_HAVE_A_TWO_BY_TWO) {
			a = caryll_from_f2dot14(caryll_blt16u(start + offset));
			b = caryll_from_f2dot14(caryll_blt16u(start + offset + 2));
			c = caryll_from_f2dot14(caryll_blt16u(start + offset + 4));
			d = caryll_from_f2dot14(caryll_blt16u(start + offset + 2));
			offset += 8;
		}
		g->references[j].glyph.gid = index;
		g->references[j].a = a;
		g->references[j].b = b;
		g->references[j].c = c;
		g->references[j].d = d;
		g->references[j].x = x;
		g->references[j].y = y;
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

static glyf_glyph *caryll_read_glyph(font_file_pointer data, uint32_t offset) {
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
				glyf->glyphs[j] = spaceGlyph();
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

void caryll_glyphorder_to_json(table_glyf *table, json_value *root) {
	if (!table) return;
	json_value *order = json_array_new(table->numberGlyphs);
	for (uint16_t j = 0; j < table->numberGlyphs; j++) {
		json_array_push(order, json_string_new_length(sdslen(table->glyphs[j]->name), table->glyphs[j]->name));
	}
	json_object_push(root, "glyph_order", order);
}

json_value *coord_to_json(float z) {
	if (roundf(z) == z) {
		return json_integer_new(z);
	} else {
		return json_double_new(z);
	}
}
void caryll_glyf_to_json(table_glyf *table, json_value *root) {
	if (!table) return;
	json_value *glyf = json_object_new(table->numberGlyphs);
	for (uint16_t j = 0; j < table->numberGlyphs; j++) {
		glyf_glyph *g = table->glyphs[j];
		json_value *glyph = json_object_new(4);
		json_object_push(glyph, "name", json_string_new_length(sdslen(g->name), g->name));
		json_object_push(glyph, "advanceWidth", json_integer_new(g->advanceWidth));
		// contours
		{
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
			json_object_push(glyph, "contours", contours);
		}
		// references
		{
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
		}
		// instructions
		{
			json_value *instructions = json_array_new(g->instructionsLength);
			for (uint16_t j = 0; j < g->instructionsLength; j++) {
				json_array_push(instructions, json_integer_new(g->instructions[j]));
			}
			json_object_push(glyph, "instructions", instructions);
		}
		json_object_push(glyf, g->name, glyph);
	}
	json_object_push(root, "glyf", glyf);

	caryll_glyphorder_to_json(table, root);
}
