#include "GSUB-GPOS.h"

table_otl *caryll_new_otl() {
	table_otl *table = malloc(sizeof(table_otl));
	table->nLanguages = 0;
	table->languages = NULL;
	return table;
}
void caryll_delete_otl(table_otl *table) {
	if (!table) return;
	if (table->languages) {
		for (uint16_t j = 0; j < table->nLanguages; j++) {
			if (table->languages[j].name) sdsfree(table->languages[j].name);
			if (table->languages[j].featureIndex) free(table->languages[j].featureIndex);
		}
	}
	free(table);
}

#define checkLength(offset)                                                                                            \
	if (tableLength < offset) { goto FAIL; }

void parseLanguage(font_file_pointer data, uint32_t tableLength, uint32_t base, otl_language_system *lang) {
	checkLength(base + 6);
	lang->reqFeatureIndex = caryll_blt16u(data + base + 2);
	lang->featureCount = caryll_blt16u(data + base + 4);
	checkLength(base + 6 + lang->featureCount * 2);
	
	lang->featureIndex = malloc(lang->featureCount * sizeof(uint16_t));
	if (!lang->featureIndex) goto FAIL;
	for (uint16_t j = 0; j < lang->featureCount; j++) {
		lang->featureIndex[j] = caryll_blt16u(data + base + 6 + 2 * j);
	}
	return;
FAIL:
	if (lang->featureIndex) free(lang->featureIndex);
	lang->featureCount = 0;
	lang->reqFeatureIndex = 0xFFFF;
	return;
}

table_otl *caryll_read_otl_common(font_file_pointer data, uint32_t tableLength) {
	table_otl *table = caryll_new_otl();
	if (!table) goto FAIL;
	checkLength(10);
	uint16_t scriptListOffset = caryll_blt16u(data + 4);
	checkLength(scriptListOffset + 2);
	uint16_t featureListOffset = caryll_blt16u(data + 6);
	checkLength(featureListOffset);
	uint16_t lookupListOffset = caryll_blt16u(data + 8);
	checkLength(lookupListOffset);

	// parse script list
	uint16_t scriptCount = caryll_blt16u(data + scriptListOffset);
	checkLength(scriptListOffset + 2 + 6 * scriptCount);

	uint32_t nLanguageCombinations = 0;
	for (uint16_t j = 0; j < scriptCount; j++) {
		uint16_t scriptOffset = scriptListOffset + caryll_blt16u(data + scriptListOffset + 2 + 6 * j + 4);
		checkLength(scriptOffset + 4);

		uint16_t defaultLangSystem = caryll_blt16u(data + scriptOffset);
		nLanguageCombinations += (defaultLangSystem ? 1 : 0) + caryll_blt16u(data + scriptOffset + 2);
	}

	table->nLanguages = nLanguageCombinations;

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
			parseLanguage(data, tableLength, scriptOffset + defaultLangSystem, &(languages[currentLang]));
			currentLang += 1;
		}
		uint16_t langSysCount = caryll_blt16u(data + scriptOffset + 2);
		for (uint16_t k = 0; k < langSysCount; k++) {
			uint32_t langTag = caryll_blt32u(data + scriptOffset + 4 + 6 * k);
			uint16_t langSys = caryll_blt16u(data + scriptOffset + 4 + 6 * k + 4);
			languages[currentLang].name = sdscatprintf(
			    sdsempty(), "%c%c%c%c-%c%c%c%c", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff,
			    (langTag >> 24) & 0xFF, (langTag >> 16) & 0xFF, (langTag >> 8) & 0xff, langTag & 0xff);
			parseLanguage(data, tableLength, scriptOffset + langSys, &(languages[currentLang]));
			currentLang += 1;
		}
	}

	table->languages = languages;

	for (uint16_t j = 0; j < nLanguageCombinations; j++) {
		fprintf(stderr, "%s %d %d\n", languages[j].name, languages[j].reqFeatureIndex, languages[j].featureCount);
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
