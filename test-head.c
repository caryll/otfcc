#include <stdio.h>
#include <stdlib.h>

#include "caryll-sfnt.h"
#include "caryll-font.h"

#define PATTERN_PROPERTY_NAME "%30s"
#define PATTERN_DEC    (PATTERN_PROPERTY_NAME " = %10u\n")
#define PATTERN_SIGNED (PATTERN_PROPERTY_NAME " = %10d\n")
#define PATTERN_HEX    (PATTERN_PROPERTY_NAME " = %#010x\n")

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
	caryll_font_close(font);
	caryll_sfnt_close(sfnt);
	return 0;
}
