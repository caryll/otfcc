#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

glyf_glyph caryll_read_simple_glyph(font_file_pointer start, uint16_t numberOfContours) {
	glyf_glyph glyph;
	glyph.numberOfContours = numberOfContours;
	glyph.numberOfReferences = 0;

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
	glyph.instructionsLength = instructionLength;
	glyph.instructions = instructions;

	// read points
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
		if (currentContourPointIndex >= contours[currentContour].pointsCount) {
			currentContourPointIndex = 0;
			currentContour += 1;
		}
		contours[currentContour].points[currentContourPointIndex++].onCurve = (flag & GLYF_FLAG_ON_CURVE);
		if (flag & GLYF_FLAG_REPEAT) {
			uint8_t repeat = flagStart[flagBytesReadSofar];
			flagBytesReadSofar += 1;
			for (uint8_t j = 0; j < repeat; j++) {
				if (currentContourPointIndex >= contours[currentContour].pointsCount) {
					currentContourPointIndex = 0;
					currentContour += 1;
				}
				flags[flagsReadSofar + j] = flag;
				contours[currentContour].points[currentContourPointIndex++].onCurve = (flag & GLYF_FLAG_ON_CURVE);
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
		if (currentContourPointIndex >= contours[currentContour].pointsCount) {
			currentContourPointIndex = 0;
			currentContour += 1;
		}
		contours[currentContour].points[currentContourPointIndex++].x = x;
		coordinatesRead += 1;
	}
	// read Y
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
		if (currentContourPointIndex >= contours[currentContour].pointsCount) {
			currentContourPointIndex = 0;
			currentContour += 1;
		}
		contours[currentContour].points[currentContourPointIndex++].y = y;
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
	glyph.content.contours = contours;
	return glyph;
}

glyf_glyph caryll_read_composite_glyph(font_file_pointer start) {
	glyf_glyph glyph;
	glyph.numberOfContours = 0;
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
	glyph.numberOfReferences = numberOfReferences;
	glyph.content.references = (glyf_reference *)malloc(sizeof(glyf_reference) * numberOfReferences);
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
		glyph.content.references[j].index = index;
		glyph.content.references[j].a = a;
		glyph.content.references[j].b = b;
		glyph.content.references[j].c = c;
		glyph.content.references[j].d = d;
		glyph.content.references[j].x = x;
		glyph.content.references[j].y = y;
	}
	if (glyphHasInstruction) {
		uint16_t instructionLength = caryll_blt16u(start + offset);
		font_file_pointer instructions = NULL;
		if (instructionLength > 0) {
			instructions = (font_file_pointer)malloc(sizeof(uint8_t) * instructionLength);
			memcpy(instructions, start + offset + 2, sizeof(uint8_t) * instructionLength);
		}
		glyph.instructionsLength = instructionLength;
		glyph.instructions = instructions;
	} else {
		glyph.instructionsLength = 0;
		glyph.instructions = NULL;
	}

	return glyph;
}

glyf_glyph caryll_read_glyph(font_file_pointer data, uint32_t offset) {
	font_file_pointer start = data + offset;
	int16_t numberOfContours = caryll_blt16u(start);
	if (numberOfContours > 0) {
		return caryll_read_simple_glyph(start + 10, numberOfContours);
	} else {
		return caryll_read_composite_glyph(start + 10);
	}
}

glyf_glyph emptyGlyph() {
	glyf_glyph g;
	g.numberOfContours = 0;
	g.numberOfReferences = 0;
	g.instructionsLength = 0;
	g.instructions = NULL;
	g.content.contours = NULL;
	g.advanceWidth = 0;
	return g;
}

void caryll_read_glyf(caryll_font *font, caryll_packet packet) {
	if (font->head == NULL || font->maxp == NULL) return;
	uint16_t locaIsLong = font->head->indexToLocFormat;
	uint16_t numGlyphs = font->maxp->numGlyphs;
	uint32_t *offsets = (uint32_t *)malloc(sizeof(uint32_t) * numGlyphs);
	bool foundLoca = false;
	// read loca
	FOR_TABLE('loca', table) {
		font_file_pointer data = table.data;
		// uint32_t length = table.length;
		for (uint32_t j = 0; j < numGlyphs; j++) {
			if (locaIsLong) {
				offsets[j] = caryll_blt32u(data + j * 4);
			} else {
				offsets[j] = caryll_blt16u(data + j * 2);
			}
		}
		foundLoca = true;
	}
	if (!foundLoca) {
		free(offsets);
		return;
	}
	FOR_TABLE('glyf', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		font->glyf = (table_glyf *)malloc(sizeof(table_glyf) * 1);
		font->glyf->numberGlyphs = numGlyphs;
		font->glyf->glyphs = malloc(sizeof(glyf_glyph) * numGlyphs);
		for (uint16_t j = 0; j < numGlyphs; j++) {
			if (length - offsets[j] > 5) {
				font->glyf->glyphs[j] = caryll_read_glyph(data, offsets[j]);
			} else {
				font->glyf->glyphs[j] = emptyGlyph();
			}
		}
	}
	// read glyf
	printf("\n");
	free(offsets);
}
