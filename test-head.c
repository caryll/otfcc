#include <stdio.h>
#include <stdlib.h>

#include "caryll-sfnt.h"
#include "caryll-font.h"

#define PATTERN_PROPERTY_NAME "%30s"
#define PATTERN_DEC (PATTERN_PROPERTY_NAME " = %10u\n")
#define PATTERN_SIGNED (PATTERN_PROPERTY_NAME " = %10d\n")
#define PATTERN_HEX (PATTERN_PROPERTY_NAME " = %#010x\n")

#define DUMP_CMAP 0
#define DUMP_GLYF 0

int main(int argc, char *argv[]) {
	caryll_sfnt *sfnt = caryll_sfnt_open(argv[1]);
	caryll_font *font = caryll_font_open(sfnt, 0);
	printf(PATTERN_HEX, "head.version", font->head->version);
	printf(PATTERN_DEC, "head.unitsPerEm", font->head->unitsPerEm);
	printf(PATTERN_SIGNED, "hhea.ascender", font->hhea->ascender);
	printf(PATTERN_SIGNED, "hhea.descender", font->hhea->descender);
	printf(PATTERN_DEC, "maxp.numGlyphs", font->maxp->numGlyphs);
	printf(PATTERN_HEX, "post.version", font->post->version);
	printf(PATTERN_DEC, "post.isFixedPitch", font->post->isFixedPitch);
	printf(PATTERN_DEC, "OS/2.version", font->OS_2->version);
	if (DUMP_GLYF)
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_glyph g = font->glyf->glyphs[j];
			printf("GLYPH %5d : width %5d, %5d contours, %5d references\n", j, g.advanceWidth, g.numberOfContours,
			       g.numberOfReferences);
			if (g.numberOfContours > 0) {
				for (uint16_t k = 0; k < g.numberOfContours; k++) {
					printf("    CONTOUR %3d : %5d points\n", k, g.content.contours[k].pointsCount);
					for (uint16_t m = 0; m < g.content.contours[k].pointsCount; m++) {
						printf("        %s (%5d, %5d)\n", g.content.contours[k].points[m].onCurve ? "ON " : "OFF",
						       g.content.contours[k].points[m].x, g.content.contours[k].points[m].y);
					}
				}
			} else if (g.numberOfReferences > 0) {
				for (uint16_t k = 0; k < g.numberOfReferences; k++) {
					printf("    REF %5d %5d %5d %5g %5g %5g %5g\n", g.content.references[k].index,
					       g.content.references[k].x, g.content.references[k].y, g.content.references[k].a,
					       g.content.references[k].b, g.content.references[k].c, g.content.references[k].d);
				}
			}
		}
	if (DUMP_CMAP) {
		cmap_hash item, tmp;
		HASH_ITER(hh, *(font->cmap), item, tmp) { printf("[U+%04x -> %d]", item->unicode, item->glyph); }
	}

	caryll_font_close(font);
	caryll_sfnt_close(sfnt);
	return 0;
}
