#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_head(caryll_font *font, caryll_packet packet) {
	FOR_TABLE('head', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		if (length < 54) {
			fprintf(stderr, "table 'head' corrupted.\n");
			font->head = NULL;
		} else {
			table_head *head = (table_head *)malloc(sizeof(table_head) * 1);
			head->version = caryll_blt32u(data);
			head->fontRevison = caryll_blt32u(data + 4);
			head->checkSumAdjustment = caryll_blt32u(data + 8);
			head->magicNumber = caryll_blt32u(data + 12);
			head->flags = caryll_blt16u(data + 16);
			head->unitsPerEm = caryll_blt16u(data + 18);
			head->created = caryll_blt64u(data + 20);
			head->modified = caryll_blt64u(data + 28);
			head->xMin = caryll_blt16u(data + 36);
			head->yMin = caryll_blt16u(data + 38);
			head->xMax = caryll_blt16u(data + 40);
			head->yMax = caryll_blt16u(data + 42);
			head->macStyle = caryll_blt16u(data + 44);
			head->lowestRecPPEM = caryll_blt16u(data + 46);
			head->fontDirectoryHint = caryll_blt16u(data + 48);
			head->indexToLocFormat = caryll_blt16u(data + 50);
			head->glyphDataFormat = caryll_blt16u(data + 52);
			font->head = head;
		}
	}
}

void caryll_head_to_json(caryll_font *font, json_value *root) {
	if (!font->head) return;
	json_value *head = json_object_new(15);
	json_object_push(head, "version", json_integer_new(font->head->version));
	json_object_push(head, "fontRevison", json_integer_new(font->head->fontRevison));
	json_object_push(head, "flags", json_integer_new(font->head->flags));
	json_object_push(head, "unitsPerEm", json_integer_new(font->head->unitsPerEm));
	json_object_push(head, "created", json_integer_new(font->head->created));
	json_object_push(head, "modified", json_integer_new(font->head->modified));
	json_object_push(head, "xMin", json_integer_new(font->head->xMin));
	json_object_push(head, "xMax", json_integer_new(font->head->xMax));
	json_object_push(head, "yMin", json_integer_new(font->head->yMin));
	json_object_push(head, "yMax", json_integer_new(font->head->yMax));
	json_object_push(head, "macStyle", json_integer_new(font->head->macStyle));
	json_object_push(head, "lowestRecPPEM", json_integer_new(font->head->lowestRecPPEM));
	json_object_push(head, "fontDirectoryHint", json_integer_new(font->head->fontDirectoryHint));
	json_object_push(head, "indexToLocFormat", json_integer_new(font->head->indexToLocFormat));
	json_object_push(head, "glyphDataFormat", json_integer_new(font->head->glyphDataFormat));
	json_object_push(root, "head", head);
}

