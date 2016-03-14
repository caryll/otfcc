#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_OS_2(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'OS/2') {
			font_file_pointer data = packet.pieces[i].data;
			uint32_t length = packet.pieces[i].length;
			if (length < 2) {
				printf("table 'OS/2' corrupted.\n");
				font->OS_2 = NULL;
				return;
			}
			table_OS_2 *os_2 = (table_OS_2 *)malloc(sizeof(table_OS_2) * 1);
			os_2->version = caryll_blt16u(data);
			// version 1
			if (os_2->version == 0 || (os_2->version >= 1 && length < 86)) {
				printf("table 'OS/2' corrupted or deprecated.\n");
				free(os_2);
				font->OS_2 = NULL;
				return;
			}
			if (os_2->version >= 1) {
				os_2->xAvgCharWidth = caryll_blt16u(data + 2);
				os_2->usWeightClass = caryll_blt16u(data + 4);
				os_2->usWidthClass = caryll_blt16u(data + 6);
				os_2->fsType = caryll_blt16u(data + 8);
				os_2->ySubscriptXSize = caryll_blt16u(data + 10);
				os_2->ySubscriptYSize = caryll_blt16u(data + 12);
				os_2->ySubscriptXOffset = caryll_blt16u(data + 14);
				os_2->ySubscriptYOffset = caryll_blt16u(data + 16);
				os_2->ySupscriptXSize = caryll_blt16u(data + 18);
				os_2->ySupscriptYSize = caryll_blt16u(data + 20);
				os_2->ySupscriptXOffset = caryll_blt16u(data + 22);
				os_2->ySupscriptYOffset = caryll_blt16u(data + 24);
				os_2->yStrikeoutSize = caryll_blt16u(data + 26);
				os_2->yStrikeoutPosition = caryll_blt16u(data + 28);
				os_2->sFamilyClass = caryll_blt16u(data + 30);
				memcpy(os_2->panose, data + 32, 10);
				os_2->ulUnicodeRange1 = caryll_blt32u(data + 42);
				os_2->ulUnicodeRange2 = caryll_blt32u(data + 46);
				os_2->ulUnicodeRange3 = caryll_blt32u(data + 50);
				os_2->ulUnicodeRange4 = caryll_blt32u(data + 54);
				memcpy(os_2->achVendID, data + 58, 4);
				os_2->fsSelection = caryll_blt16u(data + 62);
				os_2->usFirstCharIndex = caryll_blt16u(data + 64);
				os_2->usLastCharIndex = caryll_blt16u(data + 66);
				os_2->sTypoAscender = caryll_blt16u(data + 68);
				os_2->sTypoDescender = caryll_blt16u(data + 70);
				os_2->sTypoLineGap = caryll_blt16u(data + 72);
				os_2->usWinAscent = caryll_blt16u(data + 74);
				os_2->usWinDescent = caryll_blt16u(data + 76);
				os_2->ulCodePageRange1 = caryll_blt32u(data + 78);
				os_2->ulCodePageRange2 = caryll_blt32u(data + 82);
			}
			// version 2, 3, 4
			if (os_2->version >= 2 && length < 96) {
				printf("table 'OS/2' corrupted.\n");
				free(os_2);
				font->OS_2 = NULL;
				return;
			}
			if (os_2->version >= 2) {
				os_2->sxHeight = caryll_blt16u(data + 86);
				os_2->sCapHeight = caryll_blt16u(data + 88);
				os_2->usDefaultChar = caryll_blt16u(data + 90);
				os_2->usBreakChar = caryll_blt16u(data + 92);
				os_2->usMaxContext = caryll_blt16u(data + 94);
			}
			// version 5
			if (os_2->version >= 5 && length < 100) {
				printf("table 'OS/2' corrupted.\n");
				free(os_2);
				font->OS_2 = NULL;
				return;
			}
			if (os_2->version >= 5) {
				os_2->usLowerOpticalPointSize = caryll_blt16u(data + 96);
				os_2->usLowerOpticalPointSize = caryll_blt16u(data + 98);
			}
			font->OS_2 = os_2;
		}
	}
}
