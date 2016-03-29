#include "stat.h"

// Stating

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
		if (g->verticalOrigin - g->stat.yMin > maxExtent) maxExtent = g->verticalOrigin - g->stat.yMin;
	}
	font->vhea->numOfLongVerMetrics = count_a;
	font->vhea->minTop = minTSB;
	font->vhea->minBottom = minBSB;
	font->vhea->yMaxExtent = maxExtent;
	font->vhea->advanceHeightMax = maxHeight;
	font->vmtx = vmtx;
}
void caryll_font_stat(caryll_font *font) {
	if (font->glyf && font->head) caryll_stat_glyf(font);
	if (font->glyf && font->maxp && font->maxp->version == 0x10000) caryll_stat_maxp(font);
	if (font->fpgm && font->maxp && font->fpgm->length > font->maxp->maxSizeOfInstructions) {
		font->maxp->maxSizeOfInstructions = font->fpgm->length;
	}
	if (font->prep && font->maxp && font->prep->length > font->maxp->maxSizeOfInstructions) {
		font->maxp->maxSizeOfInstructions = font->prep->length;
	}
	if (font->glyf && font->hhea) caryll_font_stat_hmtx(font);
	if (font->glyf && font->vhea) caryll_font_stat_vmtx(font);
}
