#include "otl.h"

table_otl *caryll_new_otl() {
	table_otl *table = malloc(sizeof(table_otl));
	table->languageCount = 0;
	table->languages = NULL;
	table->featureCount = 0;
	table->features = NULL;
	table->lookupCount = 0;
	table->lookups = NULL;
	return table;
}
void caryll_delete_coverage(otl_coverage *coverage) {
	if (coverage && coverage->glyphs) free(coverage->glyphs);
	if (coverage) free(coverage);
}

void caryll_delete_otl(table_otl *table) {
	if (!table) return;
	if (table->languages) {
		for (uint16_t j = 0; j < table->languageCount; j++) {
			if (table->languages[j].name) sdsfree(table->languages[j].name);
			if (table->languages[j].features) free(table->languages[j].features);
		}
		free(table->languages);
	}
	if (table->features) {
		for (uint16_t j = 0; j < table->featureCount; j++) {
			if (table->features[j].name) sdsfree(table->features[j].name);
			if (table->features[j].lookups) free(table->features[j].lookups);
		}
		free(table->features);
	}
	if (table->lookups) {
		for (uint16_t j = 0; j < table->lookupCount; j++) {
			switch (table->lookups[j].type) {
				case otl_type_gsub_single:
					caryll_delete_gsub_single(&(table->lookups[j]));
				default:
					break;
			}
		}
		free(table->lookups);
	}
	free(table);
}

#define checkLength(offset)                                                                                            \
	if (tableLength < offset) { goto FAIL; }

void parseLanguage(font_file_pointer data, uint32_t tableLength, uint32_t base, otl_language_system *lang,
                   uint16_t featureCount, otl_feature *features) {
	checkLength(base + 6);
	uint16_t rid = caryll_blt16u(data + base + 2);
	if (rid < featureCount) {
		lang->requiredFeature = &(features[rid]);
	} else {
		lang->requiredFeature = NULL;
	}
	lang->featureCount = caryll_blt16u(data + base + 4);
	checkLength(base + 6 + lang->featureCount * 2);

	lang->features = malloc(lang->featureCount * sizeof(otl_feature *));
	if (!lang->features) goto FAIL;
	for (uint16_t j = 0; j < lang->featureCount; j++) {
		uint16_t featureIndex = caryll_blt16u(data + base + 6 + 2 * j);
		if (featureIndex < featureCount) {
			lang->features[j] = &(features[featureIndex]);
		} else {
			lang->features[j] = NULL;
		}
	}
	return;
FAIL:
	if (lang->features) free(lang->features);
	lang->featureCount = 0;
	lang->requiredFeature = NULL;
	return;
}

table_otl *caryll_read_otl_common(font_file_pointer data, uint32_t tableLength, otl_lookup_type lookup_type_base) {
	table_otl *table = caryll_new_otl();
	if (!table) goto FAIL;
	checkLength(10);
	uint32_t scriptListOffset = caryll_blt16u(data + 4);
	checkLength(scriptListOffset + 2);
	uint32_t featureListOffset = caryll_blt16u(data + 6);
	checkLength(featureListOffset + 2);
	uint32_t lookupListOffset = caryll_blt16u(data + 8);
	checkLength(lookupListOffset + 2);

	// parse lookup list
	{
		uint16_t lookupCount = caryll_blt16u(data + lookupListOffset);
		checkLength(lookupListOffset + 2 + lookupCount * 2);
		otl_lookup *lookups = malloc(lookupCount * sizeof(otl_lookup));
		if (!lookups) goto FAIL;
		for (uint16_t j = 0; j < lookupCount; j++) {
			checkLength(lookupListOffset + lookups[j]._offset + 6);
			lookups[j].name = NULL;
			lookups[j]._offset = lookupListOffset + caryll_blt16u(data + lookupListOffset + 2 + 2 * j);
			lookups[j].type = caryll_blt16u(data + lookups[j]._offset) + lookup_type_base;
		}
		table->lookupCount = lookupCount;
		table->lookups = lookups;
	}

	// parse feature list
	{
		uint16_t featureCount = caryll_blt16u(data + featureListOffset);
		checkLength(featureListOffset + 2 + featureCount * 6);
		otl_feature *features = malloc(featureCount * sizeof(otl_feature));
		if (!features) goto FAIL;
		for (uint16_t j = 0; j < featureCount; j++) {
			uint32_t tag = caryll_blt32u(data + featureListOffset + 2 + j * 6);
			features[j].name = sdscatprintf(sdsempty(), "%c%c%c%c_%d", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
			                                (tag >> 8) & 0xff, tag & 0xff, j);
			uint32_t featureOffset = featureListOffset + caryll_blt16u(data + featureListOffset + 2 + j * 6 + 4);

			checkLength(featureOffset + 4);
			uint16_t lookupCount = caryll_blt16u(data + featureOffset + 2);
			checkLength(featureOffset + 4 + lookupCount * 2);
			features[j].lookupCount = lookupCount;
			features[j].lookups = malloc(lookupCount * sizeof(otl_feature *));
			for (uint16_t k = 0; k < lookupCount; k++) {
				uint16_t lookupid = caryll_blt16u(data + featureOffset + 4 + k * 2);
				if (lookupid < table->lookupCount) {
					features[j].lookups[k] = &(table->lookups[lookupid]);
					if (!features[j].lookups[k]->name) {
						features[j].lookups[k]->name =
						    sdscatprintf(sdsempty(), "lookup_%c%c%c%c_%d", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
						                 (tag >> 8) & 0xff, tag & 0xff, k);
					}
				}
			}
		}
		table->featureCount = featureCount;
		table->features = features;
	}

	// parse script list
	{
		uint16_t scriptCount = caryll_blt16u(data + scriptListOffset);
		checkLength(scriptListOffset + 2 + 6 * scriptCount);

		uint32_t nLanguageCombinations = 0;
		for (uint16_t j = 0; j < scriptCount; j++) {
			uint32_t scriptOffset = scriptListOffset + caryll_blt16u(data + scriptListOffset + 2 + 6 * j + 4);
			checkLength(scriptOffset + 4);

			uint16_t defaultLangSystem = caryll_blt16u(data + scriptOffset);
			nLanguageCombinations += (defaultLangSystem ? 1 : 0) + caryll_blt16u(data + scriptOffset + 2);
		}

		table->languageCount = nLanguageCombinations;

		otl_language_system *languages = malloc(nLanguageCombinations * sizeof(otl_language_system));
		if (!languages) goto FAIL;

		uint16_t currentLang = 0;
		for (uint16_t j = 0; j < scriptCount; j++) {
			uint32_t tag = caryll_blt32u(data + scriptListOffset + 2 + 6 * j);
			uint32_t scriptOffset = scriptListOffset + caryll_blt16u(data + scriptListOffset + 2 + 6 * j + 4);
			uint16_t defaultLangSystem = caryll_blt16u(data + scriptOffset);
			if (defaultLangSystem) {
				languages[currentLang].name = sdscatprintf(sdsempty(), "%c%c%c%c_DFLT", (tag >> 24) & 0xFF,
				                                           (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff);
				parseLanguage(data, tableLength, scriptOffset + defaultLangSystem, &(languages[currentLang]),
				              table->featureCount, table->features);
				currentLang += 1;
			}
			uint16_t langSysCount = caryll_blt16u(data + scriptOffset + 2);
			for (uint16_t k = 0; k < langSysCount; k++) {
				uint32_t langTag = caryll_blt32u(data + scriptOffset + 4 + 6 * k);
				uint16_t langSys = caryll_blt16u(data + scriptOffset + 4 + 6 * k + 4);
				languages[currentLang].name = sdscatprintf(
				    sdsempty(), "%c%c%c%c_%c%c%c%c", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF, (tag >> 8) & 0xff,
				    tag & 0xff, (langTag >> 24) & 0xFF, (langTag >> 16) & 0xFF, (langTag >> 8) & 0xff, langTag & 0xff);
				parseLanguage(data, tableLength, scriptOffset + langSys, &(languages[currentLang]), table->featureCount,
				              table->features);
				currentLang += 1;
			}
		}

		table->languages = languages;
	}
	// name all lookups
	for (uint16_t j = 0; j < table->lookupCount; j++) {
		if (!table->lookups[j].name)
			table->lookups[j].name = sdscatprintf(sdsempty(), "lookup_%02x_%d", table->lookups[j].type, j);
	}
	return table;
FAIL:
	if (table) caryll_delete_otl(table);
	return NULL;
}

typedef struct {
	int gid;
	UT_hash_handle hh;
} coverage_entry;

otl_coverage *caryll_read_coverage(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_coverage *coverage = malloc(sizeof(otl_coverage));
	if (!coverage) return coverage;
	coverage->numGlyphs = 0;
	coverage->glyphs = NULL;
	if (tableLength < offset + 4) return coverage;
	uint16_t format = caryll_blt16u(data + offset);
	switch (format) {
		case 1: {
			uint16_t glyphCount = caryll_blt16u(data + offset + 2);
			if (tableLength < offset + 4 + glyphCount * 2) return coverage;
			coverage_entry *hash = NULL;
			for (uint16_t j = 0; j < glyphCount; j++) {
				coverage_entry *item = NULL;
				int gid = caryll_blt16u(data + offset + 4 + j * 2);
				HASH_FIND_INT(hash, &gid, item);
				if (!item) {
					item = calloc(1, sizeof(coverage_entry));
					item->gid = gid;
					HASH_ADD_INT(hash, gid, item);
				}
			}
			coverage->numGlyphs = HASH_COUNT(hash);
			coverage->glyphs = malloc(coverage->numGlyphs * sizeof(glyph_handle));
			{
				uint16_t j = 0;
				coverage_entry *e, *tmp;
				HASH_ITER(hh, hash, e, tmp) {
					coverage->glyphs[j].gid = e->gid;
					coverage->glyphs[j].name = NULL;
					HASH_DEL(hash, e);
					free(e);
					j++;
				}
			}
			break;
		}
		case 2: {
			uint16_t rangeCount = caryll_blt16u(data + offset + 2);
			if (tableLength < offset + 4 + rangeCount * 6) return coverage;
			coverage_entry *hash = NULL;
			for (uint16_t j = 0; j < rangeCount; j++) {
				uint16_t start = caryll_blt16u(data + offset + 4 + 6 * j);
				uint16_t end = caryll_blt16u(data + offset + 4 + 6 * j + 2);
				for (int k = start; k <= end; k++) {
					coverage_entry *item = NULL;
					HASH_FIND_INT(hash, &k, item);
					if (!item) {
						item = calloc(1, sizeof(coverage_entry));
						item->gid = k;
						HASH_ADD_INT(hash, gid, item);
					}
				}
			}
			coverage->numGlyphs = HASH_COUNT(hash);
			coverage->glyphs = malloc(coverage->numGlyphs * sizeof(glyph_handle));
			{
				uint16_t j = 0;
				coverage_entry *e, *tmp;
				HASH_ITER(hh, hash, e, tmp) {
					coverage->glyphs[j].gid = e->gid;
					coverage->glyphs[j].name = NULL;
					HASH_DEL(hash, e);
					free(e);
					j++;
				}
			}
			break;
		}
		default:
			break;
	}
	return coverage;
}

void caryll_read_otl_lookup(font_file_pointer data, uint32_t tableLength, otl_lookup *lookup) {
	lookup->subtableCount = caryll_blt16u(data + lookup->_offset + 4);
	if (tableLength < lookup->_offset + 6 + 2 * lookup->subtableCount) {
		lookup->subtableCount = 0;
		lookup->subtables = NULL;
		return;
	}
	lookup->subtables = malloc(lookup->subtableCount * sizeof(otl_subtable *));
	switch (lookup->type) {
		case otl_type_gsub_single:
			caryll_read_gsub_single(data, tableLength, lookup);
			break;
		default:
			lookup->type = otl_type_unknown;
			if (lookup->subtables) free(lookup->subtables);
			lookup->subtables = NULL;
			break;
	}
}

table_otl *caryll_read_otl(caryll_packet packet, uint32_t tag) {
	table_otl *otl = NULL;
	FOR_TABLE(tag, table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		otl = caryll_read_otl_common(
		    data, length,
		    (tag == 'GSUB' ? otl_type_gsub_unknown : tag == 'GPOS' ? otl_type_gpos_unknown : otl_type_unknown));
		if (!otl) goto FAIL;
		for (uint16_t j = 0; j < otl->lookupCount; j++) {
			caryll_read_otl_lookup(data, length, &otl->lookups[j]);
		}
		return otl;
	FAIL:
		if (otl) caryll_delete_otl(otl);
		otl = NULL;
	}
	return NULL;
}
json_value *caryll_coverage_to_json(otl_coverage *coverage) {
	json_value *a = json_array_new(coverage->numGlyphs);
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		json_array_push(a, json_string_new(coverage->glyphs[j].name));
	}
	return preserialize(a);
}
void caryll_lookup_to_json(otl_lookup *lookup, json_value *dump) {
	switch (lookup->type) {
		case otl_type_gsub_single:
			caryll_gsub_single_to_json(lookup, dump);
			break;
		default:
			break;
	}
}
void caryll_otl_to_json(table_otl *table, json_value *root, caryll_dump_options *dumpopts, const char *tag) {
	if (!table || !table->languages || !table->lookups || !table->features) return;
	json_value *otl = json_object_new(3);
	{
		// dump script list
		json_value *languages = json_object_new(table->languageCount);
		for (uint16_t j = 0; j < table->languageCount; j++) {
			json_value *language = json_object_new(5);
			if (table->languages[j].requiredFeature) {
				json_object_push(language, "requiredFeature",
				                 json_string_new(table->languages[j].requiredFeature->name));
			}
			json_value *features = json_array_new(table->languages[j].featureCount);
			for (uint16_t k = 0; k < table->languages[j].featureCount; k++)
				if (table->languages[j].features[k]) {
					json_array_push(features, json_string_new(table->languages[j].features[k]->name));
				}
			json_object_push(language, "features", features);
			json_object_push(languages, table->languages[j].name, language);
		}
		json_object_push(otl, "languages", languages);
	}
	{
		// dump feature list
		json_value *features = json_object_new(table->featureCount);
		for (uint16_t j = 0; j < table->featureCount; j++) {
			json_value *feature = json_array_new(table->features[j].lookupCount);
			for (uint16_t k = 0; k < table->features[j].lookupCount; k++)
				if (table->features[j].lookups[k]) {
					json_array_push(feature, json_string_new(table->features[j].lookups[k]->name));
				}
			json_object_push(features, table->features[j].name, feature);
		}
		json_object_push(otl, "features", features);
	}
	{
		// dump lookups
		json_value *lookups = json_object_new(table->lookupCount);
		for (uint16_t j = 0; j < table->lookupCount; j++) {
			json_value *lookup = json_object_new(5);
			caryll_lookup_to_json(&table->lookups[j], lookup);
			json_object_push(lookups, table->lookups[j].name, lookup);
		}
		json_object_push(otl, "lookups", lookups);
	}
	json_object_push(root, tag, otl);
}
