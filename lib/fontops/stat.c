#include "stat.h"
#include <time.h>

// Stating

typedef enum { stat_not_started = 0, stat_doing = 1, stat_completed = 2 } stat_status;

glyf_glyph_stat stat_single_glyph(table_glyf *table, glyf_reference *gr, stat_status *stated, uint8_t depth,
                                  uint16_t topj) {
	glyf_glyph_stat stat = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint16_t j = gr->glyph.gid;
	if (depth >= 0xFF) return stat;
	if (stated[j] == stat_doing) {
		// We have a circular reference
		fprintf(stderr, "[Stat] Circular glyph reference found in gid %d to "
		                "gid %d. The reference will be dropped.\n",
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

void caryll_stat_glyf(caryll_font *font) {
	stat_status *stated = calloc(font->glyf->numberGlyphs, sizeof(stat_status));
	float xmin = 0xFFFF;
	float xmax = -0xFFFF;
	float ymin = 0xFFFF;
	float ymax = -0xFFFF;
	for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
		glyf_reference gr;
		gr.glyph.gid = j;
		gr.glyph.name = NULL;
		gr.x = 0;
		gr.y = 0;
		gr.a = 1;
		gr.b = 0;
		gr.c = 0;
		gr.d = 1;
		glyf_glyph_stat thatstat = font->glyf->glyphs[j]->stat = stat_single_glyph(font->glyf, &gr, stated, 0, j);
		if (thatstat.xMin < xmin) xmin = thatstat.xMin;
		if (thatstat.xMax > xmax) xmax = thatstat.xMax;
		if (thatstat.yMin < ymin) ymin = thatstat.yMin;
		if (thatstat.yMax > ymax) ymax = thatstat.yMax;
	}
	font->head->xMin = xmin;
	font->head->xMax = xmax;
	font->head->yMin = ymin;
	font->head->yMax = ymax;
	free(stated);
}

void caryll_stat_maxp(caryll_font *font) {
	uint16_t nestDepth = 0;
	uint16_t nPoints = 0;
	uint16_t nContours = 0;
	uint16_t nComponents = 0;
	uint16_t nCompositePoints = 0;
	uint16_t nCompositeContours = 0;
	uint16_t instSize = 0;
	for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
		glyf_glyph *g = font->glyf->glyphs[j];
		if (g->numberOfContours > 0) {
			if (g->stat.nPoints > nPoints) nPoints = g->stat.nPoints;
			if (g->stat.nContours > nContours) nContours = g->stat.nContours;
		} else if (g->numberOfReferences > 0) {
			if (g->stat.nCompositePoints > nCompositePoints) nCompositePoints = g->stat.nCompositePoints;
			if (g->stat.nCompositeContours > nCompositeContours) nCompositeContours = g->stat.nCompositeContours;
			if (g->stat.nestDepth > nestDepth) nestDepth = g->stat.nestDepth;
			if (g->numberOfReferences > nComponents) nComponents = g->numberOfReferences;
		}
		if (g->instructionsLength > instSize) instSize = g->instructionsLength;
	}
	font->maxp->maxPoints = nPoints;
	font->maxp->maxContours = nContours;
	font->maxp->maxCompositePoints = nCompositePoints;
	font->maxp->maxCompositeContours = nCompositeContours;
	font->maxp->maxComponentDepth = nestDepth;
	font->maxp->maxComponentElements = nComponents;
	font->maxp->maxSizeOfInstructions = instSize;
}

void caryll_font_stat_hmtx(caryll_font *font) {
	if (!font->glyf) return;
	table_hmtx *hmtx = malloc(sizeof(table_hmtx) * 1);
	if (!hmtx) return;
	uint16_t count_a = font->glyf->numberGlyphs;
	while (count_a > 2 &&
	       font->glyf->glyphs[count_a - 1]->advanceWidth == font->glyf->glyphs[count_a - 2]->advanceWidth) {
		count_a--;
	}
	int16_t count_k = font->glyf->numberGlyphs - count_a;
	hmtx->metrics = malloc(sizeof(horizontal_metric) * count_a);
	if (count_k > 0) {
		hmtx->leftSideBearing = malloc(sizeof(int16_t) * count_k);
	} else {
		hmtx->leftSideBearing = NULL;
	}
	int16_t minLSB = 0x7FFF;
	int16_t minRSB = 0x7FFF;
	int16_t maxExtent = -0x8000;
	uint16_t maxWidth = 0;
	for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
		uint16_t advanceWidth = font->glyf->glyphs[j]->advanceWidth;
		int16_t lsb = font->glyf->glyphs[j]->stat.xMin;
		int16_t rsb = advanceWidth - font->glyf->glyphs[j]->stat.xMax;

		if (j < count_a) {
			hmtx->metrics[j].advanceWidth = font->glyf->glyphs[j]->advanceWidth;
			hmtx->metrics[j].lsb = font->glyf->glyphs[j]->stat.xMin;
		} else {
			hmtx->leftSideBearing[j - count_a] = lsb;
		}

		if (advanceWidth > maxWidth) maxWidth = advanceWidth;
		if (lsb < minLSB) minLSB = lsb;
		if (rsb < minRSB) minRSB = rsb;
		if (font->glyf->glyphs[j]->stat.xMax > maxExtent) maxExtent = font->glyf->glyphs[j]->stat.xMax;
	}
	font->hhea->numberOfMetrics = count_a;
	font->hhea->minLeftSideBearing = minLSB;
	font->hhea->minRightSideBearing = minRSB;
	font->hhea->xMaxExtent = maxExtent;
	font->hhea->advanceWithMax = maxWidth;
	font->hmtx = hmtx;
}
void caryll_font_stat_vmtx(caryll_font *font) {
	if (!font->glyf) return;
	table_vmtx *vmtx = malloc(sizeof(table_vmtx) * 1);
	if (!vmtx) return;
	uint16_t count_a = font->glyf->numberGlyphs;
	while (count_a > 2 &&
	       font->glyf->glyphs[count_a - 1]->advanceHeight == font->glyf->glyphs[count_a - 2]->advanceHeight) {
		count_a--;
	}
	int16_t count_k = font->glyf->numberGlyphs - count_a;
	vmtx->metrics = malloc(sizeof(vertical_metric) * count_a);
	if (count_k > 0) {
		vmtx->topSideBearing = malloc(sizeof(int16_t) * count_k);
	} else {
		vmtx->topSideBearing = NULL;
	}

	int16_t minTSB = 0x7FFF;
	int16_t minBSB = 0x7FFF;
	int16_t maxExtent = -0x8000;
	uint16_t maxHeight = 0;
	for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
		glyf_glyph *g = font->glyf->glyphs[j];
		uint16_t advanceHeight = g->advanceHeight;
		int16_t tsb = g->verticalOrigin - g->stat.yMax;
		int16_t bsb = g->stat.yMin - g->verticalOrigin + g->advanceHeight;
		if (j < count_a) {
			vmtx->metrics[j].advanceHeight = advanceHeight;
			vmtx->metrics[j].tsb = tsb;
		} else {
			vmtx->topSideBearing[j - count_a] = tsb;
		}
		if (advanceHeight > maxHeight) maxHeight = advanceHeight;
		if (tsb < minTSB) minTSB = tsb;
		if (bsb < minBSB) minBSB = bsb;
		if (g->verticalOrigin - g->stat.yMin > maxExtent) { maxExtent = g->verticalOrigin - g->stat.yMin; }
	}
	font->vhea->numOfLongVerMetrics = count_a;
	font->vhea->minTop = minTSB;
	font->vhea->minBottom = minBSB;
	font->vhea->yMaxExtent = maxExtent;
	font->vhea->advanceHeightMax = maxHeight;
	font->vmtx = vmtx;
}
void caryll_font_stat_OS_2(caryll_font *font, caryll_dump_options *dumpopts) {
	cmap_entry *item;
	// Stat for OS/2.ulUnicodeRange.
	uint32_t u1 = 0;
	uint32_t u2 = 0;
	uint32_t u3 = 0;
	uint32_t u4 = 0;
	int minUnicode = 0xFFFF;
	int maxUnicode = 0;
	foreach_hash(item, *font->cmap) {
		int u = item->unicode;
		// Stat for minimium and maximium unicode
		if (u < minUnicode) minUnicode = u;
		if (u > maxUnicode) maxUnicode = u;
		// Reference: https://www.microsoft.com/typography/otspec/os2.htm#ur
		if ((u >= 0x0000 && u <= 0x007F)) { u1 |= (1 << 0); }
		if ((u >= 0x0080 && u <= 0x00FF)) { u1 |= (1 << 1); }
		if ((u >= 0x0100 && u <= 0x017F)) { u1 |= (1 << 2); }
		if ((u >= 0x0180 && u <= 0x024F)) { u1 |= (1 << 3); }
		if ((u >= 0x0250 && u <= 0x02AF) || (u >= 0x1D00 && u <= 0x1D7F) || (u >= 0x1D80 && u <= 0x1DBF)) {
			u1 |= (1 << 4);
		}
		if ((u >= 0x02B0 && u <= 0x02FF) || (u >= 0xA700 && u <= 0xA71F)) { u1 |= (1 << 5); }
		if ((u >= 0x0300 && u <= 0x036F) || (u >= 0x1DC0 && u <= 0x1DFF)) { u1 |= (1 << 6); }
		if ((u >= 0x0370 && u <= 0x03FF)) { u1 |= (1 << 7); }
		if ((u >= 0x2C80 && u <= 0x2CFF)) { u1 |= (1 << 8); }
		if ((u >= 0x0400 && u <= 0x04FF) || (u >= 0x0500 && u <= 0x052F) || (u >= 0x2DE0 && u <= 0x2DFF) ||
		    (u >= 0xA640 && u <= 0xA69F)) {
			u1 |= (1 << 9);
		}
		if ((u >= 0x0530 && u <= 0x058F)) { u1 |= (1 << 10); }
		if ((u >= 0x0590 && u <= 0x05FF)) { u1 |= (1 << 11); }
		if ((u >= 0xA500 && u <= 0xA63F)) { u1 |= (1 << 12); }
		if ((u >= 0x0600 && u <= 0x06FF) || (u >= 0x0750 && u <= 0x077F)) { u1 |= (1 << 13); }
		if ((u >= 0x07C0 && u <= 0x07FF)) { u1 |= (1 << 14); }
		if ((u >= 0x0900 && u <= 0x097F)) { u1 |= (1 << 15); }
		if ((u >= 0x0980 && u <= 0x09FF)) { u1 |= (1 << 16); }
		if ((u >= 0x0A00 && u <= 0x0A7F)) { u1 |= (1 << 17); }
		if ((u >= 0x0A80 && u <= 0x0AFF)) { u1 |= (1 << 18); }
		if ((u >= 0x0B00 && u <= 0x0B7F)) { u1 |= (1 << 19); }
		if ((u >= 0x0B80 && u <= 0x0BFF)) { u1 |= (1 << 20); }
		if ((u >= 0x0C00 && u <= 0x0C7F)) { u1 |= (1 << 21); }
		if ((u >= 0x0C80 && u <= 0x0CFF)) { u1 |= (1 << 22); }
		if ((u >= 0x0D00 && u <= 0x0D7F)) { u1 |= (1 << 23); }
		if ((u >= 0x0E00 && u <= 0x0E7F)) { u1 |= (1 << 24); }
		if ((u >= 0x0E80 && u <= 0x0EFF)) { u1 |= (1 << 25); }
		if ((u >= 0x10A0 && u <= 0x10FF) || (u >= 0x2D00 && u <= 0x2D2F)) { u1 |= (1 << 26); }
		if ((u >= 0x1B00 && u <= 0x1B7F)) { u1 |= (1 << 27); }
		if ((u >= 0x1100 && u <= 0x11FF)) { u1 |= (1 << 28); }
		if ((u >= 0x1E00 && u <= 0x1EFF) || (u >= 0x2C60 && u <= 0x2C7F) || (u >= 0xA720 && u <= 0xA7FF)) {
			u1 |= (1 << 29);
		}
		if ((u >= 0x1F00 && u <= 0x1FFF)) { u1 |= (1 << 30); }
		if ((u >= 0x2000 && u <= 0x206F) || (u >= 0x2E00 && u <= 0x2E7F)) { u1 |= (1 << 31); }
		if ((u >= 0x2070 && u <= 0x209F)) { u2 |= (1 << 0); }
		if ((u >= 0x20A0 && u <= 0x20CF)) { u2 |= (1 << 1); }
		if ((u >= 0x20D0 && u <= 0x20FF)) { u2 |= (1 << 2); }
		if ((u >= 0x2100 && u <= 0x214F)) { u2 |= (1 << 3); }
		if ((u >= 0x2150 && u <= 0x218F)) { u2 |= (1 << 4); }
		if ((u >= 0x2190 && u <= 0x21FF) || (u >= 0x27F0 && u <= 0x27FF) || (u >= 0x2900 && u <= 0x297F) ||
		    (u >= 0x2B00 && u <= 0x2BFF)) {
			u2 |= (1 << 5);
		}
		if ((u >= 0x2200 && u <= 0x22FF) || (u >= 0x2A00 && u <= 0x2AFF) || (u >= 0x27C0 && u <= 0x27EF) ||
		    (u >= 0x2980 && u <= 0x29FF)) {
			u2 |= (1 << 6);
		}
		if ((u >= 0x2300 && u <= 0x23FF)) { u2 |= (1 << 7); }
		if ((u >= 0x2400 && u <= 0x243F)) { u2 |= (1 << 8); }
		if ((u >= 0x2440 && u <= 0x245F)) { u2 |= (1 << 9); }
		if ((u >= 0x2460 && u <= 0x24FF)) { u2 |= (1 << 10); }
		if ((u >= 0x2500 && u <= 0x257F)) { u2 |= (1 << 11); }
		if ((u >= 0x2580 && u <= 0x259F)) { u2 |= (1 << 12); }
		if ((u >= 0x25A0 && u <= 0x25FF)) { u2 |= (1 << 13); }
		if ((u >= 0x2600 && u <= 0x26FF)) { u2 |= (1 << 14); }
		if ((u >= 0x2700 && u <= 0x27BF)) { u2 |= (1 << 15); }
		if ((u >= 0x3000 && u <= 0x303F)) { u2 |= (1 << 16); }
		if ((u >= 0x3040 && u <= 0x309F)) { u2 |= (1 << 17); }
		if ((u >= 0x30A0 && u <= 0x30FF) || (u >= 0x31F0 && u <= 0x31FF)) { u2 |= (1 << 18); }
		if ((u >= 0x3100 && u <= 0x312F) || (u >= 0x31A0 && u <= 0x31BF)) { u2 |= (1 << 19); }
		if ((u >= 0x3130 && u <= 0x318F)) { u2 |= (1 << 20); }
		if ((u >= 0xA840 && u <= 0xA87F)) { u2 |= (1 << 21); }
		if ((u >= 0x3200 && u <= 0x32FF)) { u2 |= (1 << 22); }
		if ((u >= 0x3300 && u <= 0x33FF)) { u2 |= (1 << 23); }
		if ((u >= 0xAC00 && u <= 0xD7AF)) { u2 |= (1 << 24); }
		if ((u >= 0xD800 && u <= 0xDFFF) || u > 0xFFFF) { u2 |= (1 << 25); }
		if ((u >= 0x10900 && u <= 0x1091F)) { u2 |= (1 << 26); }
		if ((u >= 0x4E00 && u <= 0x9FFF) || (u >= 0x2E80 && u <= 0x2EFF) || (u >= 0x2F00 && u <= 0x2FDF) ||
		    (u >= 0x2FF0 && u <= 0x2FFF) || (u >= 0x3400 && u <= 0x4DBF) || (u >= 0x20000 && u <= 0x2F7FF) ||
		    (u >= 0x3190 && u <= 0x319F)) {
			u2 |= (1 << 27);
		}
		if ((u >= 0xE000 && u <= 0xF8FF)) { u2 |= (1 << 28); }
		if ((u >= 0x31C0 && u <= 0x31EF) || (u >= 0xF900 && u <= 0xFAFF) || (u >= 0x2F800 && u <= 0x2FA1F)) {
			u2 |= (1 << 29);
		}
		if ((u >= 0xFB00 && u <= 0xFB4F)) { u2 |= (1 << 30); }
		if ((u >= 0xFB50 && u <= 0xFDFF)) { u2 |= (1 << 31); }
		if ((u >= 0xFE20 && u <= 0xFE2F)) { u3 |= (1 << 0); }
		if ((u >= 0xFE10 && u <= 0xFE1F) || (u >= 0xFE30 && u <= 0xFE4F)) { u3 |= (1 << 1); }
		if ((u >= 0xFE50 && u <= 0xFE6F)) { u3 |= (1 << 2); }
		if ((u >= 0xFE70 && u <= 0xFEFF)) { u3 |= (1 << 3); }
		if ((u >= 0xFF00 && u <= 0xFFEF)) { u3 |= (1 << 4); }
		if ((u >= 0xFFF0 && u <= 0xFFFF)) { u3 |= (1 << 5); }
		if ((u >= 0x0F00 && u <= 0x0FFF)) { u3 |= (1 << 6); }
		if ((u >= 0x0700 && u <= 0x074F)) { u3 |= (1 << 7); }
		if ((u >= 0x0780 && u <= 0x07BF)) { u3 |= (1 << 8); }
		if ((u >= 0x0D80 && u <= 0x0DFF)) { u3 |= (1 << 9); }
		if ((u >= 0x1000 && u <= 0x109F)) { u3 |= (1 << 10); }
		if ((u >= 0x1200 && u <= 0x137F) || (u >= 0x1380 && u <= 0x139F) || (u >= 0x2D80 && u <= 0x2DDF)) {
			u3 |= (1 << 11);
		}
		if ((u >= 0x13A0 && u <= 0x13FF)) { u3 |= (1 << 12); }
		if ((u >= 0x1400 && u <= 0x167F)) { u3 |= (1 << 13); }
		if ((u >= 0x1680 && u <= 0x169F)) { u3 |= (1 << 14); }
		if ((u >= 0x16A0 && u <= 0x16FF)) { u3 |= (1 << 15); }
		if ((u >= 0x1780 && u <= 0x17FF) || (u >= 0x19E0 && u <= 0x19FF)) { u3 |= (1 << 16); }
		if ((u >= 0x1800 && u <= 0x18AF)) { u3 |= (1 << 17); }
		if ((u >= 0x2800 && u <= 0x28FF)) { u3 |= (1 << 18); }
		if ((u >= 0xA000 && u <= 0xA48F) || (u >= 0xA490 && u <= 0xA4CF)) { u3 |= (1 << 19); }
		if ((u >= 0x1700 && u <= 0x171F) || (u >= 0x1720 && u <= 0x173F) || (u >= 0x1740 && u <= 0x175F) ||
		    (u >= 0x1760 && u <= 0x177F)) {
			u3 |= (1 << 20);
		}
		if ((u >= 0x10300 && u <= 0x1032F)) { u3 |= (1 << 21); }
		if ((u >= 0x10330 && u <= 0x1034F)) { u3 |= (1 << 22); }
		if ((u >= 0x10400 && u <= 0x1044F)) { u3 |= (1 << 23); }
		if ((u >= 0x1D000 && u <= 0x1D0FF) || (u >= 0x1D100 && u <= 0x1D1FF) || (u >= 0x1D200 && u <= 0x1D24F)) {
			u3 |= (1 << 24);
		}
		if ((u >= 0x1D400 && u <= 0x1D7FF)) { u3 |= (1 << 25); }
		if ((u >= 0xFF000 && u <= 0xFFFFD) || (u >= 0x100000 && u <= 0x10FFFD)) { u3 |= (1 << 26); }
		if ((u >= 0xFE00 && u <= 0xFE0F) || (u >= 0xE0100 && u <= 0xE01EF)) { u3 |= (1 << 27); }
		if ((u >= 0xE0000 && u <= 0xE007F)) { u3 |= (1 << 28); }
		if ((u >= 0x1900 && u <= 0x194F)) { u3 |= (1 << 29); }
		if ((u >= 0x1950 && u <= 0x197F)) { u3 |= (1 << 30); }
		if ((u >= 0x1980 && u <= 0x19DF)) { u3 |= (1 << 31); }
		if ((u >= 0x1A00 && u <= 0x1A1F)) { u4 |= (1 << 0); }
		if ((u >= 0x2C00 && u <= 0x2C5F)) { u4 |= (1 << 1); }
		if ((u >= 0x2D30 && u <= 0x2D7F)) { u4 |= (1 << 2); }
		if ((u >= 0x4DC0 && u <= 0x4DFF)) { u4 |= (1 << 3); }
		if ((u >= 0xA800 && u <= 0xA82F)) { u4 |= (1 << 4); }
		if ((u >= 0x10000 && u <= 0x1007F) || (u >= 0x10080 && u <= 0x100FF) || (u >= 0x10100 && u <= 0x1013F)) {
			u4 |= (1 << 5);
		}
		if ((u >= 0x10140 && u <= 0x1018F)) { u4 |= (1 << 6); }
		if ((u >= 0x10380 && u <= 0x1039F)) { u4 |= (1 << 7); }
		if ((u >= 0x103A0 && u <= 0x103DF)) { u4 |= (1 << 8); }
		if ((u >= 0x10450 && u <= 0x1047F)) { u4 |= (1 << 9); }
		if ((u >= 0x10480 && u <= 0x104AF)) { u4 |= (1 << 10); }
		if ((u >= 0x10800 && u <= 0x1083F)) { u4 |= (1 << 11); }
		if ((u >= 0x10A00 && u <= 0x10A5F)) { u4 |= (1 << 12); }
		if ((u >= 0x1D300 && u <= 0x1D35F)) { u4 |= (1 << 13); }
		if ((u >= 0x12000 && u <= 0x123FF) || (u >= 0x12400 && u <= 0x1247F)) { u4 |= (1 << 14); }
		if ((u >= 0x1D360 && u <= 0x1D37F)) { u4 |= (1 << 15); }
		if ((u >= 0x1B80 && u <= 0x1BBF)) { u4 |= (1 << 16); }
		if ((u >= 0x1C00 && u <= 0x1C4F)) { u4 |= (1 << 17); }
		if ((u >= 0x1C50 && u <= 0x1C7F)) { u4 |= (1 << 18); }
		if ((u >= 0xA880 && u <= 0xA8DF)) { u4 |= (1 << 19); }
		if ((u >= 0xA900 && u <= 0xA92F)) { u4 |= (1 << 20); }
		if ((u >= 0xA930 && u <= 0xA95F)) { u4 |= (1 << 21); }
		if ((u >= 0xAA00 && u <= 0xAA5F)) { u4 |= (1 << 22); }
		if ((u >= 0x10190 && u <= 0x101CF)) { u4 |= (1 << 23); }
		if ((u >= 0x101D0 && u <= 0x101FF)) { u4 |= (1 << 24); }
		if ((u >= 0x102A0 && u <= 0x102DF) || (u >= 0x10280 && u <= 0x1029F) || (u >= 0x10920 && u <= 0x1093F)) {
			u4 |= (1 << 25);
		}
		if ((u >= 0x1F030 && u <= 0x1F09F) || (u >= 0x1F000 && u <= 0x1F02F)) { u4 |= (1 << 26); }
	}
	font->OS_2->ulUnicodeRange1 = u1;
	font->OS_2->ulUnicodeRange2 = u2;
	font->OS_2->ulUnicodeRange3 = u3;
	font->OS_2->ulUnicodeRange4 = u4;
	if (minUnicode < 0x10000) {
		font->OS_2->usFirstCharIndex = minUnicode;
	} else {
		font->OS_2->usFirstCharIndex = 0xFFFF;
	}
	if (maxUnicode < 0x10000) {
		font->OS_2->usLastCharIndex = maxUnicode;
	} else {
		font->OS_2->usLastCharIndex = 0xFFFF;
	}

	if (!dumpopts->keep_average_char_width) {
		uint32_t totalWidth = 0;
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) { totalWidth += font->glyf->glyphs[j]->advanceWidth; }
		font->OS_2->xAvgCharWidth = totalWidth / font->glyf->numberGlyphs;
	}
}
static void caryll_stat_cff_widths(caryll_font *font) {
	if (!font->glyf || !font->CFF_) return;
	// Stat the most frequent character width
	uint32_t *frequency = calloc(2000, sizeof(uint32_t));
	for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
		if (font->glyf->glyphs[j]->advanceWidth < 2000) { frequency[font->glyf->glyphs[j]->advanceWidth] += 1; }
	}
	uint16_t maxfreq = 0;
	uint16_t maxj = 0;
	for (uint16_t j = 0; j < 2000; j++) {
		if (frequency[j] > maxfreq) {
			maxfreq = frequency[j];
			maxj = j;
		}
	}
	// stat nominalWidthX
	uint16_t nn = 0;
	uint32_t nnsum = 0;
	for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
		if (font->glyf->glyphs[j]->advanceWidth != maxj) {
			nn += 1;
			nnsum += font->glyf->glyphs[j]->advanceWidth;
		}
	}
	font->CFF_->privateDict->defaultWidthX = maxj;
	if (nn != 0) { font->CFF_->privateDict->nominalWidthX = nnsum / nn; }
	if (font->CFF_->fdArray) {
		for (uint16_t j = 0; j < font->CFF_->fdArrayCount; j++) {
			font->CFF_->fdArray[j]->privateDict->defaultWidthX = maxj;
			font->CFF_->fdArray[j]->privateDict->nominalWidthX = font->CFF_->privateDict->nominalWidthX;
		}
	}
}

void caryll_font_stat(caryll_font *font, caryll_dump_options *dumpopts) {
	if (font->glyf && font->head) {
		caryll_stat_glyf(font);
		if (!dumpopts->keep_modified_time) { font->head->modified = 2082844800 + (int64_t)time(NULL); }
	}
	if (font->head && font->CFF_) {
		font->CFF_->fontBBoxBottom = font->head->yMin;
		font->CFF_->fontBBoxTop = font->head->yMax;
		font->CFF_->fontBBoxLeft = font->head->xMin;
		font->CFF_->fontBBoxRight = font->head->xMax;
		if (font->glyf && font->CFF_->isCID) { font->CFF_->cidCount = font->glyf->numberGlyphs; }
		caryll_stat_cff_widths(font);
	}
	if (font->glyf && font->maxp) { font->maxp->numGlyphs = font->glyf->numberGlyphs; }
	if (font->glyf && font->post) { font->post->maxMemType42 = font->glyf->numberGlyphs; }
	if (font->glyf && font->maxp && font->maxp->version == 0x10000) caryll_stat_maxp(font);
	if (font->fpgm && font->maxp && font->fpgm->length > font->maxp->maxSizeOfInstructions) {
		font->maxp->maxSizeOfInstructions = font->fpgm->length;
	}
	if (font->prep && font->maxp && font->prep->length > font->maxp->maxSizeOfInstructions) {
		font->maxp->maxSizeOfInstructions = font->prep->length;
	}
	if (font->OS_2 && font->cmap && font->glyf) caryll_font_stat_OS_2(font, dumpopts);
	if (font->subtype == FONTTYPE_TTF) {
		if (font->maxp) font->maxp->version = 0x00010000;
	} else {
		if (font->maxp) font->maxp->version = 0x00005000;
	}
	if (font->glyf && font->hhea) caryll_font_stat_hmtx(font);
	if (font->glyf && font->vhea) caryll_font_stat_vmtx(font);
}
