#include <stdio.h>
#include <stdlib.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"

#include "kit.h"

int main(int argc, char *argv[]) {
	printf("Testing Payload %s\n", argv[1]);
	caryll_sfnt *sfnt = caryll_sfnt_open(argv[1]);
	caryll_font *font = caryll_font_open(sfnt, 0);
	
	int nChecks = 0;
	
	assert_equal("head.version", font->head->version, 0x10000);
	assert_equal("OS/2.version", font->OS_2->version, 0x0004);
	assert_equal("OS/2.ulUnicodeRange2", font->OS_2->ulUnicodeRange2, 0x2adf3c10);
	
	caryll_font_close(font);
	caryll_sfnt_close(sfnt);
	return 0;
}
