#include "head.h"

table_head *caryll_new_head() {
	table_head *head = (table_head *)calloc(1, sizeof(table_head));
	head->magicNumber = 0x5f0f3cf5;
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
			head->version = read_32s(data);
			head->fontRevison = read_32u(data + 4);
			head->checkSumAdjustment = read_32u(data + 8);
			head->magicNumber = read_32u(data + 12);
			head->flags = read_16u(data + 16);
			head->unitsPerEm = read_16u(data + 18);
			head->created = read_64u(data + 20);
			head->modified = read_64u(data + 28);
			head->xMin = read_16u(data + 36);
			head->yMin = read_16u(data + 38);
			head->xMax = read_16u(data + 40);
			head->yMax = read_16u(data + 42);
			head->macStyle = read_16u(data + 44);
			head->lowestRecPPEM = read_16u(data + 46);
			head->fontDirectoryHint = read_16u(data + 48);
			head->indexToLocFormat = read_16u(data + 50);
			head->glyphDataFormat = read_16u(data + 52);
			return head;
		}
	}
	return NULL;
}

static const char *headFlagsLabels[] = {"baseline_at_y_0",
                                        "lsb_at_x_0",
                                        "instr_may_depend_on_point_size",
                                        "do_not_use_fraction_size",
                                        "instr_may_alter_advance_width",
                                        "designed_for_vertical",
                                        "_reserved1",
                                        "designed_for_complex_script",
                                        "has_metamorphosis_effects",
                                        "contains_strong_rtl",
                                        "contains_indic_rearrangement",
                                        "font_is_lossless",
                                        "font_is_converted",
                                        "optimized_for_cleartype",
                                        "last_resort_font"};
static const char *macStyleLabels[] = {"bold", "italic", "underline", "outline", "shadow",
                                       "condensed"
                                       "extended"};
void caryll_head_to_json(table_head *table, json_value *root, caryll_dump_options *dumpopts) {
	if (!table) return;
	json_value *head = json_object_new(15);
	json_object_push(head, "version", json_double_new(caryll_from_fixed(table->version)));
	json_object_push(head, "fontRevison", json_double_new(caryll_from_fixed(table->fontRevison)));
	json_object_push(head, "flags", caryll_flags_to_json(table->flags, headFlagsLabels));
	json_object_push(head, "unitsPerEm", json_integer_new(table->unitsPerEm));
	json_object_push(head, "created", json_integer_new(table->created));
	json_object_push(head, "modified", json_integer_new(table->modified));
	json_object_push(head, "xMin", json_integer_new(table->xMin));
	json_object_push(head, "xMax", json_integer_new(table->xMax));
	json_object_push(head, "yMin", json_integer_new(table->yMin));
	json_object_push(head, "yMax", json_integer_new(table->yMax));
	json_object_push(head, "macStyle", caryll_flags_to_json(table->macStyle, macStyleLabels));
	json_object_push(head, "lowestRecPPEM", json_integer_new(table->lowestRecPPEM));
	json_object_push(head, "fontDirectoryHint", json_integer_new(table->fontDirectoryHint));
	json_object_push(head, "indexToLocFormat", json_integer_new(table->indexToLocFormat));
	json_object_push(head, "glyphDataFormat", json_integer_new(table->glyphDataFormat));
	json_object_push(root, "head", head);
}

table_head *caryll_head_from_json(json_value *root, caryll_dump_options *dumpopts) {
	table_head *head = caryll_new_head();
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "head", json_object))) {
		head->version = caryll_to_fixed(json_obj_getnum_fallback(table, "version", 0));
		head->fontRevison = caryll_to_fixed(json_obj_getnum_fallback(table, "fontRevison", 0));
		head->flags = caryll_flags_from_json(json_obj_get(table, "flags"), headFlagsLabels);
		head->unitsPerEm = json_obj_getnum_fallback(table, "unitsPerEm", 0);
		head->created = json_obj_getnum_fallback(table, "created", 0);
		head->modified = json_obj_getnum_fallback(table, "modified", 0);
		head->xMin = json_obj_getnum_fallback(table, "xMin", 0);
		head->xMax = json_obj_getnum_fallback(table, "xMax", 0);
		head->yMin = json_obj_getnum_fallback(table, "yMin", 0);
		head->yMax = json_obj_getnum_fallback(table, "yMax", 0);
		head->macStyle = caryll_flags_from_json(json_obj_get(table, "macStyle"), macStyleLabels);
		head->lowestRecPPEM = json_obj_getnum_fallback(table, "lowestRecPPEM", 0);
		head->fontDirectoryHint = json_obj_getnum_fallback(table, "fontDirectoryHint", 0);
		head->indexToLocFormat = json_obj_getnum_fallback(table, "indexToLocFormat", 0);
		head->glyphDataFormat = json_obj_getnum_fallback(table, "glyphDataFormat", 0);
	}
	return head;
}

caryll_buffer *caryll_write_head(table_head *head) {
	caryll_buffer *buf = bufnew();
	if (!head) return buf;
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
