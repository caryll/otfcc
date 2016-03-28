#include "head.h"

table_head *caryll_head_new(){
	table_head *head = (table_head *)calloc(1, sizeof(table_head));
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
