#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_OS_2(caryll_font *font, caryll_packet packet) {
	table_OS_2 *os_2 = NULL;
	FOR_TABLE('OS/2', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 2) goto OS_2_CORRUPTED;
		os_2 = (table_OS_2 *)calloc(1, sizeof(table_OS_2));
		os_2->version = caryll_blt16u(data);
		// version 1
		if (os_2->version == 0 || (os_2->version >= 1 && length < 86)) goto OS_2_CORRUPTED;
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
		if (os_2->version >= 2 && length < 96) goto OS_2_CORRUPTED;
		if (os_2->version >= 2) {
			os_2->sxHeight = caryll_blt16u(data + 86);
			os_2->sCapHeight = caryll_blt16u(data + 88);
			os_2->usDefaultChar = caryll_blt16u(data + 90);
			os_2->usBreakChar = caryll_blt16u(data + 92);
			os_2->usMaxContext = caryll_blt16u(data + 94);
		}
		// version 5
		if (os_2->version >= 5 && length < 100) goto OS_2_CORRUPTED;
		if (os_2->version >= 5) {
			os_2->usLowerOpticalPointSize = caryll_blt16u(data + 96);
			os_2->usLowerOpticalPointSize = caryll_blt16u(data + 98);
		}
		font->OS_2 = os_2;
	}
	return;

OS_2_CORRUPTED:
	fprintf(stderr, "table 'OS/2' corrupted.\n");
	if (os_2 != NULL) free(os_2);
	font->OS_2 = NULL;
	return;
}

void caryll_OS_2_to_json(caryll_font *font, JSON_Object *root) {
	if (!font->OS_2) return;
	json_object_dotset_number(root, "OS_2.version", font->OS_2->version);
	json_object_dotset_number(root, "OS_2.xAvgCharWidth", font->OS_2->xAvgCharWidth);
	json_object_dotset_number(root, "OS_2.usWeightClass", font->OS_2->usWeightClass);
	json_object_dotset_number(root, "OS_2.usWidthClass", font->OS_2->usWidthClass);
	json_object_dotset_number(root, "OS_2.fsType", font->OS_2->fsType);
	json_object_dotset_number(root, "OS_2.ySubscriptXSize", font->OS_2->ySubscriptXSize);
	json_object_dotset_number(root, "OS_2.ySubscriptYSize", font->OS_2->ySubscriptYSize);
	json_object_dotset_number(root, "OS_2.ySubscriptXOffset", font->OS_2->ySubscriptXOffset);
	json_object_dotset_number(root, "OS_2.ySubscriptYOffset", font->OS_2->ySubscriptYOffset);
	json_object_dotset_number(root, "OS_2.ySupscriptXSize", font->OS_2->ySupscriptXSize);
	json_object_dotset_number(root, "OS_2.ySupscriptXOffset", font->OS_2->ySupscriptXOffset);
	json_object_dotset_number(root, "OS_2.ySupscriptYOffset", font->OS_2->ySupscriptYOffset);
	json_object_dotset_number(root, "OS_2.yStrikeoutSize", font->OS_2->yStrikeoutSize);
	json_object_dotset_number(root, "OS_2.yStrikeoutPosition", font->OS_2->yStrikeoutPosition);
	json_object_dotset_number(root, "OS_2.sFamilyClass", font->OS_2->sFamilyClass);

	JSON_Value *_panose = json_value_init_array();
	JSON_Array *panose = json_value_get_array(_panose);
	for (uint8_t j = 0; j < 10; j++) {
		json_array_append_number(panose, font->OS_2->panose[j]);
	}
	json_object_dotset_value(root, "OS_2.panose", _panose);

	json_object_dotset_number(root, "OS_2.ulUnicodeRange1", font->OS_2->ulUnicodeRange1);
	json_object_dotset_number(root, "OS_2.ulUnicodeRange2", font->OS_2->ulUnicodeRange2);
	json_object_dotset_number(root, "OS_2.ulUnicodeRange3", font->OS_2->ulUnicodeRange3);
	json_object_dotset_number(root, "OS_2.ulUnicodeRange4", font->OS_2->ulUnicodeRange4);

	sds vendorid = sdsnewlen(font->OS_2->achVendID, 4);
	json_object_dotset_string(root, "OS_2.achVendID", vendorid);
	sdsfree(vendorid);

	json_object_dotset_number(root, "OS_2.fsSelection", font->OS_2->fsSelection);
	json_object_dotset_number(root, "OS_2.usFirstCharIndex", font->OS_2->usFirstCharIndex);
	json_object_dotset_number(root, "OS_2.usLastCharIndex", font->OS_2->usLastCharIndex);
	json_object_dotset_number(root, "OS_2.sTypoAscender", font->OS_2->sTypoAscender);
	json_object_dotset_number(root, "OS_2.sTypoDescender", font->OS_2->sTypoDescender);
	json_object_dotset_number(root, "OS_2.sTypoLineGap", font->OS_2->sTypoLineGap);
	json_object_dotset_number(root, "OS_2.usWinAscent", font->OS_2->usWinAscent);
	json_object_dotset_number(root, "OS_2.usWinDescent", font->OS_2->usWinDescent);
	json_object_dotset_number(root, "OS_2.ulCodePageRange1", font->OS_2->ulCodePageRange1);
	json_object_dotset_number(root, "OS_2.ulCodePageRange2", font->OS_2->ulCodePageRange2);
	json_object_dotset_number(root, "OS_2.sxHeight", font->OS_2->sxHeight);
	json_object_dotset_number(root, "OS_2.sCapHeight", font->OS_2->sCapHeight);
	json_object_dotset_number(root, "OS_2.usDefaultChar", font->OS_2->usDefaultChar);
	json_object_dotset_number(root, "OS_2.usBreakChar", font->OS_2->usBreakChar);
	json_object_dotset_number(root, "OS_2.usMaxContext", font->OS_2->usMaxContext);
	json_object_dotset_number(root, "OS_2.usLowerOpticalPointSize", font->OS_2->usLowerOpticalPointSize);
	json_object_dotset_number(root, "OS_2.usUpperOpticalPointSize", font->OS_2->usUpperOpticalPointSize);
}
