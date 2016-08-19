#include "BASE.h"

static void delete_base_axis(base_axis *axis) {
	if (!axis) return;
	if (axis->entries) {
		for (uint16_t j = 0; j < axis->scriptCount; j++) {
			if (axis->entries[j].baseValues) free(axis->entries[j].baseValues);
		}
		free(axis->entries);
	}
}

void caryll_delete_BASE(table_BASE *base) {
	delete_base_axis(base->horizontal);
	delete_base_axis(base->vertical);
}

static int16_t read_basevalue(font_file_pointer data, uint32_t tableLength, uint16_t offset) {
	checkLength(offset + 4);
	return read_16s(data + offset + 2);
FAIL:
	return 0;
}

static void read_basescript(font_file_pointer data, uint32_t tableLength, uint16_t offset, base_script_entry *entry,
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
				entry->baseValues[j].coordinate = read_basevalue(data, tableLength, baseValuesOffset + _valOffset);
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

static base_axis *read_axis(font_file_pointer data, uint32_t tableLength, uint16_t offset) {
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
			read_basescript(data, tableLength, baseScriptListOffset + baseScriptOffset, &(axis->entries[j]),
			                baseTagList, nBaseTags);
		} else {
			axis->entries[j].baseValuesCount = 0;
			axis->entries[j].baseValues = NULL;
			axis->entries[j].defaultBaselineTag = 0;
		}
	}
	return axis;

FAIL:
	if (baseTagList) FREE(baseTagList);
	DELETE(delete_base_axis, axis);
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
		if (offsetH) base->horizontal = read_axis(data, tableLength, offsetH);
		uint16_t offsetV = read_16u(data + 6);
		if (offsetV) base->vertical = read_axis(data, tableLength, offsetV);
		return base;
	FAIL:
		DELETE(caryll_delete_BASE, base);
	}
	return base;
}

static json_value *axis_to_json(base_axis *axis) {
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
	if (base->horizontal) json_object_push(_base, "horizontal", axis_to_json(base->horizontal));
	if (base->vertical) json_object_push(_base, "vertical", axis_to_json(base->vertical));
	json_object_push(root, "BASE", _base);
}
