#include "OS_2.h"

table_OS_2 *caryll_OS_2_new() {
	table_OS_2 *os_2 = (table_OS_2 *)calloc(1, sizeof(table_OS_2));
	return os_2;
}

table_OS_2 *caryll_read_OS_2(caryll_packet packet) {
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
			os_2->sTypoAscender = caryll_blt16s(data + 68);
			os_2->sTypoDescender = caryll_blt16s(data + 70);
			os_2->sTypoLineGap = caryll_blt16s(data + 72);
			os_2->usWinAscent = caryll_blt16u(data + 74);
			os_2->usWinDescent = caryll_blt16u(data + 76);
			os_2->ulCodePageRange1 = caryll_blt32u(data + 78);
			os_2->ulCodePageRange2 = caryll_blt32u(data + 82);
		}
		// version 2, 3, 4
		if (os_2->version >= 2 && length < 96) goto OS_2_CORRUPTED;
		if (os_2->version >= 2) {
			os_2->sxHeight = caryll_blt16s(data + 86);
			os_2->sCapHeight = caryll_blt16s(data + 88);
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
		return os_2;

	OS_2_CORRUPTED:
		fprintf(stderr, "table 'OS/2' corrupted.\n");
		if (os_2 != NULL) free(os_2);
	}
	return NULL;
}

void caryll_OS_2_to_json(table_OS_2 *table, json_value *root, caryll_dump_options dumpopts) {
	if (!table) return;
	json_value *os_2 = json_object_new(30);
	json_object_push(os_2, "version", json_integer_new(table->version));
	json_object_push(os_2, "xAvgCharWidth", json_integer_new(table->xAvgCharWidth));
	json_object_push(os_2, "usWeightClass", json_integer_new(table->usWeightClass));
	json_object_push(os_2, "usWidthClass", json_integer_new(table->usWidthClass));
	json_object_push(os_2, "fsType", json_integer_new(table->fsType));
	json_object_push(os_2, "ySubscriptXSize", json_integer_new(table->ySubscriptXSize));
	json_object_push(os_2, "ySubscriptYSize", json_integer_new(table->ySubscriptYSize));
	json_object_push(os_2, "ySubscriptXOffset", json_integer_new(table->ySubscriptXOffset));
	json_object_push(os_2, "ySubscriptYOffset", json_integer_new(table->ySubscriptYOffset));
	json_object_push(os_2, "ySupscriptXSize", json_integer_new(table->ySupscriptXSize));
	json_object_push(os_2, "ySupscriptXOffset", json_integer_new(table->ySupscriptXOffset));
	json_object_push(os_2, "ySupscriptYOffset", json_integer_new(table->ySupscriptYOffset));
	json_object_push(os_2, "yStrikeoutSize", json_integer_new(table->yStrikeoutSize));
	json_object_push(os_2, "yStrikeoutPosition", json_integer_new(table->yStrikeoutPosition));
	json_object_push(os_2, "sFamilyClass", json_integer_new(table->sFamilyClass));

	json_value *panose = json_array_new(10);
	for (uint8_t j = 0; j < 10; j++) {
		json_array_push(panose, json_integer_new(table->panose[j]));
	}
	json_object_push(os_2, "panose", panose);

	json_object_push(os_2, "ulUnicodeRange1", json_integer_new(table->ulUnicodeRange1));
	json_object_push(os_2, "ulUnicodeRange2", json_integer_new(table->ulUnicodeRange2));
	json_object_push(os_2, "ulUnicodeRange3", json_integer_new(table->ulUnicodeRange3));
	json_object_push(os_2, "ulUnicodeRange4", json_integer_new(table->ulUnicodeRange4));

	sds vendorid = sdsnewlen(table->achVendID, 4);
	json_object_push(os_2, "achVendID", json_string_new(vendorid));
	sdsfree(vendorid);

	json_object_push(os_2, "fsSelection", json_integer_new(table->fsSelection));
	json_object_push(os_2, "usFirstCharIndex", json_integer_new(table->usFirstCharIndex));
	json_object_push(os_2, "usLastCharIndex", json_integer_new(table->usLastCharIndex));
	json_object_push(os_2, "sTypoAscender", json_integer_new(table->sTypoAscender));
	json_object_push(os_2, "sTypoDescender", json_integer_new(table->sTypoDescender));
	json_object_push(os_2, "sTypoLineGap", json_integer_new(table->sTypoLineGap));
	json_object_push(os_2, "usWinAscent", json_integer_new(table->usWinAscent));
	json_object_push(os_2, "usWinDescent", json_integer_new(table->usWinDescent));
	json_object_push(os_2, "ulCodePageRange1", json_integer_new(table->ulCodePageRange1));
	json_object_push(os_2, "ulCodePageRange2", json_integer_new(table->ulCodePageRange2));
	json_object_push(os_2, "sxHeight", json_integer_new(table->sxHeight));
	json_object_push(os_2, "sCapHeight", json_integer_new(table->sCapHeight));
	json_object_push(os_2, "usDefaultChar", json_integer_new(table->usDefaultChar));
	json_object_push(os_2, "usBreakChar", json_integer_new(table->usBreakChar));
	json_object_push(os_2, "usMaxContext", json_integer_new(table->usMaxContext));
	json_object_push(os_2, "usLowerOpticalPointSize", json_integer_new(table->usLowerOpticalPointSize));
	json_object_push(os_2, "usUpperOpticalPointSize", json_integer_new(table->usUpperOpticalPointSize));
	json_object_push(root, "OS_2", os_2);
}

table_OS_2 *caryll_OS_2_from_json(json_value *root, caryll_dump_options dumpopts) {
	table_OS_2 *os_2 = caryll_OS_2_new();
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "OS_2", json_object))) {
		os_2->version = json_obj_getnum_fallback(table, "version", 0);
		os_2->xAvgCharWidth = json_obj_getnum_fallback(table, "xAvgCharWidth", 0);
		os_2->usWeightClass = json_obj_getnum_fallback(table, "usWeightClass", 0);
		os_2->usWidthClass = json_obj_getnum_fallback(table, "usWidthClass", 0);
		os_2->fsType = json_obj_getnum_fallback(table, "fsType", 0);
		os_2->ySubscriptXSize = json_obj_getnum_fallback(table, "ySubscriptXSize", 0);
		os_2->ySubscriptYSize = json_obj_getnum_fallback(table, "ySubscriptYSize", 0);
		os_2->ySubscriptXOffset = json_obj_getnum_fallback(table, "ySubscriptXOffset", 0);
		os_2->ySubscriptYOffset = json_obj_getnum_fallback(table, "ySubscriptYOffset", 0);
		os_2->ySupscriptXSize = json_obj_getnum_fallback(table, "ySupscriptXSize", 0);
		os_2->ySupscriptXOffset = json_obj_getnum_fallback(table, "ySupscriptXOffset", 0);
		os_2->ySupscriptYOffset = json_obj_getnum_fallback(table, "ySupscriptYOffset", 0);
		os_2->yStrikeoutSize = json_obj_getnum_fallback(table, "yStrikeoutSize", 0);
		os_2->yStrikeoutPosition = json_obj_getnum_fallback(table, "yStrikeoutPosition", 0);
		os_2->sFamilyClass = json_obj_getnum_fallback(table, "sFamilyClass", 0);
		os_2->fsSelection = json_obj_getnum_fallback(table, "fsSelection", 0);
		os_2->usFirstCharIndex = json_obj_getnum_fallback(table, "usFirstCharIndex", 0);
		os_2->usLastCharIndex = json_obj_getnum_fallback(table, "usLastCharIndex", 0);
		os_2->sTypoAscender = json_obj_getnum_fallback(table, "sTypoAscender", 0);
		os_2->sTypoDescender = json_obj_getnum_fallback(table, "sTypoDescender", 0);
		os_2->sTypoLineGap = json_obj_getnum_fallback(table, "sTypoLineGap", 0);
		os_2->usWinAscent = json_obj_getnum_fallback(table, "usWinAscent", 0);
		os_2->usWinDescent = json_obj_getnum_fallback(table, "usWinDescent", 0);
		os_2->ulCodePageRange1 = json_obj_getnum_fallback(table, "ulCodePageRange1", 0);
		os_2->ulCodePageRange2 = json_obj_getnum_fallback(table, "ulCodePageRange2", 0);
		os_2->sxHeight = json_obj_getnum_fallback(table, "sxHeight", 0);
		os_2->sCapHeight = json_obj_getnum_fallback(table, "sCapHeight", 0);
		os_2->usDefaultChar = json_obj_getnum_fallback(table, "usDefaultChar", 0);
		os_2->usBreakChar = json_obj_getnum_fallback(table, "usBreakChar", 0);
		os_2->usMaxContext = json_obj_getnum_fallback(table, "usMaxContext", 0);
		os_2->usLowerOpticalPointSize = json_obj_getnum_fallback(table, "usLowerOpticalPointSize", 0);
		os_2->usUpperOpticalPointSize = json_obj_getnum_fallback(table, "usUpperOpticalPointSize", 0);
		// panose
		json_value *panose = NULL;
		if ((panose = json_obj_get_type(table, "panose", json_array))) {
			for (int j = 0; j < panose->u.array.length; j++)
				if (j >= 0 && j < 10) {
					json_value *term = panose->u.array.values[j];
					if (term->type == json_integer) {
						os_2->panose[j] = term->u.integer;
					} else if (term->type == json_double) {
						os_2->panose[j] = term->u.dbl;
					}
				}
		}
		// achVendID
		json_value *vendorid = NULL;
		if ((vendorid = json_obj_get_type(table, "achVendID", json_string))) {
			os_2->achVendID[0] = ' ';
			os_2->achVendID[1] = ' ';
			os_2->achVendID[2] = ' ';
			os_2->achVendID[3] = ' ';
			if (vendorid->u.string.length >= 4) {
				memcpy(os_2->achVendID, vendorid->u.string.ptr, 4);
			} else {
				memcpy(os_2->achVendID, vendorid->u.string.ptr, vendorid->u.string.length);
			}
		}
	}
	if (os_2->version < 1) os_2->version = 1;
	return os_2;
}

caryll_buffer *caryll_write_OS_2(table_OS_2 *os_2) {
	caryll_buffer *buf = bufnew();
	if (!os_2) return buf;
	bufwrite16b(buf, os_2->version);
	bufwrite16b(buf, os_2->xAvgCharWidth);
	bufwrite16b(buf, os_2->usWeightClass);
	bufwrite16b(buf, os_2->usWidthClass);
	bufwrite16b(buf, os_2->fsType);
	bufwrite16b(buf, os_2->ySubscriptXSize);
	bufwrite16b(buf, os_2->ySubscriptYSize);
	bufwrite16b(buf, os_2->ySubscriptXOffset);
	bufwrite16b(buf, os_2->ySubscriptYOffset);
	bufwrite16b(buf, os_2->ySupscriptXSize);
	bufwrite16b(buf, os_2->ySupscriptYSize);
	bufwrite16b(buf, os_2->ySupscriptXOffset);
	bufwrite16b(buf, os_2->ySupscriptYOffset);
	bufwrite16b(buf, os_2->yStrikeoutSize);
	bufwrite16b(buf, os_2->yStrikeoutPosition);
	bufwrite16b(buf, os_2->sFamilyClass);
	bufwrite_bytes(buf, 10, os_2->panose);
	bufwrite32b(buf, os_2->ulUnicodeRange1);
	bufwrite32b(buf, os_2->ulUnicodeRange2);
	bufwrite32b(buf, os_2->ulUnicodeRange3);
	bufwrite32b(buf, os_2->ulUnicodeRange4);
	bufwrite_bytes(buf, 4, os_2->achVendID);
	bufwrite16b(buf, os_2->fsSelection);
	bufwrite16b(buf, os_2->usFirstCharIndex);
	bufwrite16b(buf, os_2->usLastCharIndex);
	bufwrite16b(buf, os_2->sTypoAscender);
	bufwrite16b(buf, os_2->sTypoDescender);
	bufwrite16b(buf, os_2->sTypoLineGap);
	bufwrite16b(buf, os_2->usWinAscent);
	bufwrite16b(buf, os_2->usWinDescent);
	bufwrite32b(buf, os_2->ulCodePageRange1);
	bufwrite32b(buf, os_2->ulCodePageRange2);
	if(os_2->version < 2) return buf;
	bufwrite16b(buf, os_2->sxHeight);
	bufwrite16b(buf, os_2->sCapHeight);
	bufwrite16b(buf, os_2->usDefaultChar);
	bufwrite16b(buf, os_2->usBreakChar);
	bufwrite16b(buf, os_2->usMaxContext);
	if(os_2->version < 5) return buf;
	bufwrite16b(buf, os_2->usLowerOpticalPointSize);
	bufwrite16b(buf, os_2->usUpperOpticalPointSize);
	return buf;
}
