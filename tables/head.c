#include "head.h"

table_head *caryll_head_new() {
	table_head *head = (table_head *)calloc(1, sizeof(table_head));
	head->magicNumber = 0xF50F3CF5;
	head->unitsPerEm = 1000;
	return head;
}

table_head *caryll_read_head(caryll_packet packet) {
	FOR_TABLE('head', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		if (length < 54) {
			fprintf(stderr, "table 'head' corrupted.\n");
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
			return head;
		}
	}
	return NULL;
}

void caryll_head_to_json(table_head *table, json_value *root, caryll_dump_options dumpopts) {
	if (!table) return;
	json_value *head = json_object_new(15);
	json_object_push(head, "version", json_integer_new(table->version));
	json_object_push(head, "fontRevison", json_integer_new(table->fontRevison));
	json_object_push(head, "flags", json_integer_new(table->flags));
	json_object_push(head, "unitsPerEm", json_integer_new(table->unitsPerEm));
	json_object_push(head, "created", json_integer_new(table->created));
	json_object_push(head, "modified", json_integer_new(table->modified));
	json_object_push(head, "xMin", json_integer_new(table->xMin));
	json_object_push(head, "xMax", json_integer_new(table->xMax));
	json_object_push(head, "yMin", json_integer_new(table->yMin));
	json_object_push(head, "yMax", json_integer_new(table->yMax));
	json_object_push(head, "macStyle", json_integer_new(table->macStyle));
	json_object_push(head, "lowestRecPPEM", json_integer_new(table->lowestRecPPEM));
	json_object_push(head, "fontDirectoryHint", json_integer_new(table->fontDirectoryHint));
	json_object_push(head, "indexToLocFormat", json_integer_new(table->indexToLocFormat));
	json_object_push(head, "glyphDataFormat", json_integer_new(table->glyphDataFormat));
	json_object_push(root, "head", head);
}

table_head *caryll_head_from_json(json_value *root) {
	table_head *head = caryll_head_new();
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "head", json_object))) {
		head->version = json_obj_getnum_fallback(table, "version", 0);
		head->fontRevison = json_obj_getnum_fallback(table, "fontRevison", 0);
		head->flags = json_obj_getnum_fallback(table, "flags", 0);
		head->unitsPerEm = json_obj_getnum_fallback(table, "unitsPerEm", 0);
		head->created = json_obj_getnum_fallback(table, "created", 0);
		head->modified = json_obj_getnum_fallback(table, "modified", 0);
		head->xMin = json_obj_getnum_fallback(table, "xMin", 0);
		head->xMax = json_obj_getnum_fallback(table, "xMax", 0);
		head->yMin = json_obj_getnum_fallback(table, "yMin", 0);
		head->yMax = json_obj_getnum_fallback(table, "yMax", 0);
		head->macStyle = json_obj_getnum_fallback(table, "macStyle", 0);
		head->lowestRecPPEM = json_obj_getnum_fallback(table, "lowestRecPPEM", 0);
		head->fontDirectoryHint = json_obj_getnum_fallback(table, "fontDirectoryHint", 0);
		head->indexToLocFormat = json_obj_getnum_fallback(table, "indexToLocFormat", 0);
		head->glyphDataFormat = json_obj_getnum_fallback(table, "glyphDataFormat", 0);
	}
	return head;
}

caryll_buffer *caryll_write_head(table_head *head) {
	caryll_buffer *buf = bufnew();
	if(!head) return buf;
	bufwrite32b(buf, head->version);
	bufwrite32b(buf, head->fontRevison);
	bufwrite32b(buf, head->checkSumAdjustment);
	bufwrite32b(buf, head->magicNumber);
	bufwrite16b(buf, head->flags);
	bufwrite16b(buf, head->unitsPerEm);
	bufwrite64b(buf, head->created);
	bufwrite64b(buf, head->modified);
	bufwrite16b(buf, head->xMin);
	bufwrite16b(buf, head->yMin);
	bufwrite16b(buf, head->xMax);
	bufwrite16b(buf, head->yMax);
	bufwrite16b(buf, head->macStyle);
	bufwrite16b(buf, head->lowestRecPPEM);
	bufwrite16b(buf, head->fontDirectoryHint);
	bufwrite16b(buf, head->indexToLocFormat);
	bufwrite16b(buf, head->glyphDataFormat);
	return buf;
}
