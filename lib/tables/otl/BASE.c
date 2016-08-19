#include "BASE.h"

static void deleteBaseAxis(base_axis *axis) {
	if (!axis) return;
	if (axis->entries) {
		for (uint16_t j = 0; j < axis->scriptCount; j++) {
			if (axis->entries[j].baseValues) free(axis->entries[j].baseValues);
		}
		free(axis->entries);
	}
}

void caryll_delete_BASE(table_BASE *base) {
	deleteBaseAxis(base->horizontal);
	deleteBaseAxis(base->vertical);
}

static int16_t readBaseValue(font_file_pointer data, uint32_t tableLength, uint16_t offset) {
	checkLength(offset + 4);
	return read_16s(data + offset + 2);
FAIL:
	return 0;
}

static void readBaseScript(font_file_pointer data, uint32_t tableLength, uint16_t offset, base_script_entry *entry,
                           uint32_t *baseTagList, uint16_t nBaseTags) {
	entry->baseValuesCount = 0;
	entry->baseValues = NULL;
	entry->defaultBaselineTag = 0;
	checkLength(offset + 2); // care about base values only now
	uint16_t baseValuesOffset = read_16u(data + offset);
	if (baseValuesOffset) {
		baseValuesOffset += offset;
		checkLength(baseValuesOffset + 4);
		uint16_t defaultIndex = read_16u(data + baseValuesOffset) % nBaseTags;
		entry->defaultBaselineTag = baseTagList[defaultIndex];
		entry->baseValuesCount = read_16u(data + baseValuesOffset + 2);
		if (entry->baseValuesCount != nBaseTags) goto FAIL;
		checkLength(baseValuesOffset + 4 + 2 * entry->baseValuesCount);
		NEW_N(entry->baseValues, entry->baseValuesCount);
		for (uint16_t j = 0; j < entry->baseValuesCount; j++) {
			entry->baseValues[j].tag = baseTagList[j];
			uint16_t _valOffset = read_16u(data + baseValuesOffset + 4 + 2 * j);
			if (_valOffset) {
				entry->baseValues[j].coordinate = readBaseValue(data, tableLength, baseValuesOffset + _valOffset);
			} else {
				entry->baseValues[j].coordinate = 0;
			}
		}
		return;
	}
FAIL:
	entry->baseValuesCount = 0;
	if (entry->baseValues) free(entry->baseValues);
	entry->baseValues = NULL;
	entry->defaultBaselineTag = 0;
	return;
}

static base_axis *readAxis(font_file_pointer data, uint32_t tableLength, uint16_t offset) {
	base_axis *axis = NULL;
	uint32_t *baseTagList = NULL;
	checkLength(offset + 4);

	// Read BaseTagList
	uint16_t baseTagListOffset = offset + read_16u(data + offset);
	if (baseTagListOffset <= offset) goto FAIL;
	checkLength(baseTagListOffset + 2);
	uint16_t nBaseTags = read_16u(data + baseTagListOffset);
	if (!nBaseTags) goto FAIL;
	checkLength(baseTagListOffset + 2 + 4 * nBaseTags);
	NEW_N(baseTagList, nBaseTags);
	for (uint16_t j = 0; j < nBaseTags; j++) {
		baseTagList[j] = read_32u(data + baseTagListOffset + 2 + j * 4);
	}

	uint16_t baseScriptListOffset = offset + read_16u(data + offset + 2);
	if (baseScriptListOffset <= offset) goto FAIL;
	checkLength(baseScriptListOffset + 2);
	uint16_t nBaseScripts = read_16u(data + baseScriptListOffset);
	checkLength(baseScriptListOffset + 2 + 6 * nBaseScripts);
	NEW(axis);
	axis->scriptCount = nBaseScripts;
	NEW_N(axis->entries, nBaseScripts);
	for (uint16_t j = 0; j < nBaseScripts; j++) {
		axis->entries[j].tag = read_32u(data + baseScriptListOffset + 2 + 6 * j);
		uint16_t baseScriptOffset = read_16u(data + baseScriptListOffset + 2 + 6 * j + 4);
		if (baseScriptOffset) {
			readBaseScript(data, tableLength, baseScriptListOffset + baseScriptOffset, &(axis->entries[j]), baseTagList,
			               nBaseTags);
		} else {
			axis->entries[j].baseValuesCount = 0;
			axis->entries[j].baseValues = NULL;
			axis->entries[j].defaultBaselineTag = 0;
		}
	}
	return axis;

FAIL:
	if (baseTagList) FREE(baseTagList);
	DELETE(deleteBaseAxis, axis);
	return axis;
}

table_BASE *caryll_read_BASE(caryll_packet packet) {
	table_BASE *base = NULL;
	FOR_TABLE('BASE', table) {
		font_file_pointer data = table.data;
		uint32_t tableLength = table.length;
		checkLength(8);
		NEW_CLEAN(base);
		uint16_t offsetH = read_16u(data + 4);
		if (offsetH) base->horizontal = readAxis(data, tableLength, offsetH);
		uint16_t offsetV = read_16u(data + 6);
		if (offsetV) base->vertical = readAxis(data, tableLength, offsetV);
		return base;
	FAIL:
		DELETE(caryll_delete_BASE, base);
	}
	return base;
}

static json_value *axisToJson(base_axis *axis) {
	json_value *_axis = json_object_new(axis->scriptCount);
	for (uint16_t j = 0; j < axis->scriptCount; j++) {
		if (!axis->entries[j].tag) continue;
		json_value *_entry = json_object_new(3);
		if (axis->entries[j].defaultBaselineTag) {
			json_object_push(_entry, "defaultBaseline",
			                 json_string_new_nocopy(4, tag2str(axis->entries[j].defaultBaselineTag)));
		}
		json_value *_values = json_object_new(axis->entries[j].baseValuesCount);
		for (uint16_t k = 0; k < axis->entries[j].baseValuesCount; k++) {
			if (axis->entries[j].baseValues[k].tag)
				json_object_push(_values, tag2str(axis->entries[j].baseValues[k].tag),
				                 json_integer_new(axis->entries[j].baseValues[k].coordinate));
		}
		json_object_push(_entry, "baselines", _values);
		json_object_push(_axis, tag2str(axis->entries[j].tag), _entry);
	}
	return _axis;
}

void caryll_BASE_to_json(table_BASE *base, json_value *root, const caryll_options *options) {
	if (!base) return;
	if (options->verbose) fprintf(stderr, "Dumping BASE.\n");
	json_value *_base = json_object_new(2);
	if (base->horizontal) json_object_push(_base, "horizontal", axisToJson(base->horizontal));
	if (base->vertical) json_object_push(_base, "vertical", axisToJson(base->vertical));
	json_object_push(root, "BASE", _base);
}

static void baseScriptFromJson(json_value *_sr, base_script_entry *entry) {
	entry->defaultBaselineTag = str2tag(json_obj_getstr_share(_sr, "defaultBaseline"));
	json_value *_basevalues = json_obj_get_type(_sr, "baselines", json_object);
	if (!_basevalues) {
		entry->baseValuesCount = 0;
		entry->baseValues = NULL;
	} else {
		entry->baseValuesCount = _basevalues->u.object.length;
		NEW_N(entry->baseValues, entry->baseValuesCount);
		for (uint16_t j = 0; j < entry->baseValuesCount; j++) {
			entry->baseValues[j].tag = str2tag(_basevalues->u.object.values[j].name);
			entry->baseValues[j].coordinate = json_numof(_basevalues->u.object.values[j].value);
		}
	}
}

static base_axis *axisFromJson(json_value *_axis) {
	if (!_axis) return NULL;
	base_axis *axis;
	NEW(axis);
	axis->scriptCount = _axis->u.object.length;
	NEW_N(axis->entries, axis->scriptCount);
	uint16_t jj = 0;
	for (uint16_t j = 0; j < axis->scriptCount; j++) {
		if (_axis->u.object.values[j].value && _axis->u.object.values[j].value->type == json_object) {
			axis->entries[jj].tag = str2tag(_axis->u.object.values[j].name);
			baseScriptFromJson(_axis->u.object.values[j].value, &(axis->entries[jj]));
			jj++;
		}
	}
	axis->scriptCount = jj;
	return axis;
}

table_BASE *caryll_BASE_from_json(json_value *root, const caryll_options *options) {
	table_BASE *base = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "BASE", json_object))) {
		if (options->verbose) fprintf(stderr, "Parsing BASE.\n");
		NEW(base);
		base->horizontal = axisFromJson(json_obj_get_type(table, "horizontal", json_object));
		base->vertical = axisFromJson(json_obj_get_type(table, "vertical", json_object));
	}
	return base;
}

caryll_bkblock *axisToBk(base_axis *axis) {
	if (!axis) return NULL;
	struct {
		uint16_t size;
		uint32_t *items;
	} taglist;
	taglist.size = 0;
	taglist.items = NULL;

	for (uint16_t j = 0; j < axis->scriptCount; j++) {
		base_script_entry *entry = &(axis->entries[j]);
		if (entry->defaultBaselineTag) {
			bool found = false;
			for (uint16_t jk = 0; jk < taglist.size; jk++) {
				if (taglist.items[jk] == entry->defaultBaselineTag) {
					found = true;
					break;
				}
			}
			if (!found) {
				taglist.size += 1;
				if (taglist.items)
					taglist.items = realloc(taglist.items, taglist.size * sizeof(uint32_t));
				else
					taglist.items = malloc(taglist.size * sizeof(uint32_t));
				taglist.items[taglist.size - 1] = entry->defaultBaselineTag;
			}
		}
		for (uint16_t k = 0; k < entry->baseValuesCount; k++) {
			uint32_t tag = entry->baseValues[k].tag;
			bool found = false;
			for (uint16_t jk = 0; jk < taglist.size; jk++) {
				if (taglist.items[jk] == tag) {
					found = true;
					break;
				}
			}
			if (!found) {
				taglist.size += 1;
				if (taglist.items)
					taglist.items = realloc(taglist.items, taglist.size * sizeof(uint32_t));
				else
					taglist.items = malloc(taglist.size * sizeof(uint32_t));
				taglist.items[taglist.size - 1] = tag;
			}
		}
	}

	caryll_bkblock *baseTagList = new_bkblock(b16, taglist.size, bkover);
	for (uint16_t j = 0; j < taglist.size; j++) {
		bkblock_push(baseTagList, b32, taglist.items[j], bkover);
	}

	caryll_bkblock *baseScriptList = new_bkblock(b16, axis->scriptCount, bkover);
	for (uint16_t j = 0; j < axis->scriptCount; j++) {
		base_script_entry *entry = &(axis->entries[j]);
		caryll_bkblock *baseValues = new_bkblock(bkover);
		{
			uint16_t defaultIndex = 0;
			for (uint16_t m = 0; m < taglist.size; m++) {
				if (taglist.items[m] == entry->defaultBaselineTag) {
					defaultIndex = m;
					break;
				}
			}
			bkblock_push(baseValues, b16, defaultIndex, bkover);
		}
		bkblock_push(baseValues, b16, taglist.size, bkover);
		for (uint16_t m = 0; m < taglist.size; m++) {
			uint16_t found = false;
			uint16_t foundIndex = 0;
			for (uint16_t k = 0; k < entry->baseValuesCount; k++) {
				if (entry->baseValues[k].tag == taglist.items[m]) {
					found = true, foundIndex = k;
					break;
				}
			}
			if (found) {
				bkblock_push(baseValues,                                                     // base value
				             p16, new_bkblock(b16, 1,                                        // format
				                              b16, entry->baseValues[foundIndex].coordinate, // coordinate
				                              bkover),
				             bkover);
			} else {
				bkblock_push(baseValues,              // assign a zero value
				             p16, new_bkblock(b16, 1, // format
				                              b16, 0, // coordinate
				                              bkover),
				             bkover);
			}
		}
		caryll_bkblock *scriptRecord = new_bkblock(p16, baseValues, // BaseValues
		                                           p16, NULL,       // DefaultMinMax
		                                           b16, 0,          // BaseLangSysCount
		                                           bkover);
		bkblock_push(baseScriptList, b32, entry->tag, // BaseScriptTag
		             p16, scriptRecord,               // BaseScript
		             bkover);
	}
	free(taglist.items);
	return new_bkblock(p16, baseTagList,    // BaseTagList
	                   p16, baseScriptList, // BaseScriptList
	                   bkover);
}

caryll_buffer *caryll_write_BASE(table_BASE *base, const caryll_options *options) {
	caryll_bkblock *root = new_bkblock(b32, 0x10000,                    // Version
	                                   p16, axisToBk(base->horizontal), // HorizAxis
	                                   p16, axisToBk(base->vertical),   // VertAxis
	                                   bkover);
	return caryll_write_bk(root);
}
