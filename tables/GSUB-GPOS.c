#include "GSUB-GPOS.h"

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
void caryll_delete_otl(table_otl *table) {
	if (!table) return;
	if (table->languages) {
		for (uint16_t j = 0; j < table->languageCount; j++) {
			if (table->languages[j].name) sdsfree(table->languages[j].name);
			if (table->languages[j].features) free(table->languages[j].features);
		}
	}
	if (table->features) {
		for (uint16_t j = 0; j < table->featureCount; j++) {
			if (table->features[j].name) sdsfree(table->features[j].name);
			if (table->features[j].lookups) free(table->features[j].lookups);
		}
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

table_otl *caryll_read_otl_common(font_file_pointer data, uint32_t tableLength) {
	table_otl *table = caryll_new_otl();
	if (!table) goto FAIL;
	checkLength(10);
	uint16_t scriptListOffset = caryll_blt16u(data + 4);
	checkLength(scriptListOffset + 2);
	uint16_t featureListOffset = caryll_blt16u(data + 6);
	checkLength(featureListOffset + 2);
	uint16_t lookupListOffset = caryll_blt16u(data + 8);
	checkLength(lookupListOffset + 2);

	// parse lookup list
	{
		uint16_t lookupCount = caryll_blt16u(data + lookupListOffset);
		otl_lookup *lookups = malloc(lookupCount * sizeof(otl_lookup));
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
			features[j].name = sdscatprintf(sdsempty(), "%c%c%c%c", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
			                                (tag >> 8) & 0xff, tag & 0xff);
			uint16_t featureOffset = featureListOffset + caryll_blt16u(data + featureListOffset + 2 + j * 6 + 4);

			checkLength(featureOffset + 4);
			uint16_t lookupCount = caryll_blt16u(data + featureOffset + 2);
			checkLength(featureOffset + 4 + lookupCount * 2);
			features[j].lookups = malloc(lookupCount * sizeof(otl_feature *));
			for (uint16_t k = 0; k < lookupCount; k++) {
				features[j].lookups[k] = &(table->lookups[caryll_blt16u(data + featureOffset + 4 + k * 2)]);
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
			uint16_t scriptOffset = scriptListOffset + caryll_blt16u(data + scriptListOffset + 2 + 6 * j + 4);
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
			uint16_t scriptOffset = scriptListOffset + caryll_blt16u(data + scriptListOffset + 2 + 6 * j + 4);
			uint16_t defaultLangSystem = caryll_blt16u(data + scriptOffset);
			if (defaultLangSystem) {
				languages[currentLang].name = sdscatprintf(sdsempty(), "%c%c%c%c-DFLT", (tag >> 24) & 0xFF,
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
				    sdsempty(), "%c%c%c%c-%c%c%c%c", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF, (tag >> 8) & 0xff,
				    tag & 0xff, (langTag >> 24) & 0xFF, (langTag >> 16) & 0xFF, (langTag >> 8) & 0xff, langTag & 0xff);
				parseLanguage(data, tableLength, scriptOffset + langSys, &(languages[currentLang]), table->featureCount,
				              table->features);
				currentLang += 1;
			}
		}

		table->languages = languages;
	}

	for (uint16_t j = 0; j < table->languageCount; j++) {
		otl_language_system *lang = &(table->languages[j]);
		fprintf(stderr, "%s %d", lang->name, lang->featureCount);
		if (lang->requiredFeature) { fprintf(stderr, " RF %s", lang->requiredFeature->name); }
		fprintf(stderr, "\n");
		for (uint16_t k = 0; k < lang->featureCount; k++) {
			if (lang->features[k])
				fprintf(stderr, "    Feature %s (#%d)\n", lang->features[k]->name,
				        (int)(lang->features[k] - table->features));
		}
	}
	return table;
FAIL:
	if (table) caryll_delete_otl(table);
	return NULL;
}

void caryll_read_GSUB_GPOS(caryll_packet packet) {
	FOR_TABLE('GSUB', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		caryll_read_otl_common(data, length);
	}
}
