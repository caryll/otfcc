#include "private.h"

#define LOOKUP_READER(llt, fn)                                                                                         \
	case llt:                                                                                                          \
		return fn(data, tableLength, subtableOffset, options);

otl_Subtable *otfcc_readOtl_subtable(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset,
                                     otl_LookupType lookupType, const otfcc_Options *options) {
	switch (lookupType) {
		LOOKUP_READER(otl_type_gsub_single, otl_read_gsub_single);
		LOOKUP_READER(otl_type_gsub_multiple, otl_read_gsub_multi);
		LOOKUP_READER(otl_type_gsub_alternate, otl_read_gsub_multi);
		LOOKUP_READER(otl_type_gsub_ligature, otl_read_gsub_ligature);
		LOOKUP_READER(otl_type_gsub_chaining, otl_read_chaining);
		LOOKUP_READER(otl_type_gsub_reverse, otl_read_gsub_reverse);
		LOOKUP_READER(otl_type_gpos_chaining, otl_read_chaining);
		LOOKUP_READER(otl_type_gsub_context, otl_read_contextual);
		LOOKUP_READER(otl_type_gpos_context, otl_read_contextual);
		LOOKUP_READER(otl_type_gpos_single, otl_read_gpos_single);
		LOOKUP_READER(otl_type_gpos_pair, otl_read_gpos_pair);
		LOOKUP_READER(otl_type_gpos_cursive, otl_read_gpos_cursive);
		LOOKUP_READER(otl_type_gpos_markToBase, otl_read_gpos_markToSingle);
		LOOKUP_READER(otl_type_gpos_markToMark, otl_read_gpos_markToSingle);
		LOOKUP_READER(otl_type_gpos_markToLigature, otl_read_gpos_markToLigature);
		LOOKUP_READER(otl_type_gsub_extend, otfcc_readOtl_gsub_extend);
		LOOKUP_READER(otl_type_gpos_extend, otfcc_readOtl_gpos_extend);
		default:
			return NULL;
	}
}

static void parseLanguage(font_file_pointer data, uint32_t tableLength, uint32_t base, otl_LanguageSystem *lang,
                          tableid_t featureCount, otl_Feature **features) {
	checkLength(base + 6);
	tableid_t rid = read_16u(data + base + 2);
	if (rid < featureCount) {
		lang->requiredFeature = features[rid];
	} else {
		lang->requiredFeature = NULL;
	}
	lang->featureCount = read_16u(data + base + 4);
	checkLength(base + 6 + lang->featureCount * 2);

	NEW(lang->features, lang->featureCount);
	for (tableid_t j = 0; j < lang->featureCount; j++) {
		tableid_t featureIndex = read_16u(data + base + 6 + 2 * j);
		if (featureIndex < featureCount) {
			lang->features[j] = features[featureIndex];
		} else {
			lang->features[j] = NULL;
		}
	}
	return;
FAIL:
	if (lang->features) FREE(lang->features);
	lang->featureCount = 0;
	lang->requiredFeature = NULL;
	return;
}

static table_OTL *otfcc_readOtl_common(font_file_pointer data, uint32_t tableLength, otl_LookupType lookup_type_base,
                                       const otfcc_Options *options) {
	table_OTL *table = otfcc_newOtl();
	if (!table) goto FAIL;
	checkLength(10);
	uint32_t scriptListOffset = read_16u(data + 4);
	checkLength(scriptListOffset + 2);
	uint32_t featureListOffset = read_16u(data + 6);
	checkLength(featureListOffset + 2);
	uint32_t lookupListOffset = read_16u(data + 8);
	checkLength(lookupListOffset + 2);

	// parse lookup list
	{
		tableid_t lookupCount = read_16u(data + lookupListOffset);
		checkLength(lookupListOffset + 2 + lookupCount * 2);
		otl_Lookup **lookups;
		NEW(lookups, lookupCount);
		for (tableid_t j = 0; j < lookupCount; j++) {
			NEW(lookups[j]);
			lookups[j]->name = NULL;
			lookups[j]->_offset = lookupListOffset + read_16u(data + lookupListOffset + 2 + 2 * j);
			checkLength(lookups[j]->_offset + 6);
			lookups[j]->type = read_16u(data + lookups[j]->_offset) + lookup_type_base;
		}
		table->lookupCount = lookupCount;
		table->lookups = lookups;
	}

	// parse feature list
	{
		tableid_t featureCount = read_16u(data + featureListOffset);
		checkLength(featureListOffset + 2 + featureCount * 6);
		otl_Feature **features;
		NEW(features, featureCount);
		tableid_t lnk = 0;
		for (tableid_t j = 0; j < featureCount; j++) {
			otl_Feature *feature;
			NEW(feature);
			features[j] = feature;
			uint32_t tag = read_32u(data + featureListOffset + 2 + j * 6);
			if (options->glyph_name_prefix) {
				features[j]->name = sdscatprintf(sdsempty(), "%c%c%c%c_%s_%05d", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
				                                 (tag >> 8) & 0xff, tag & 0xff, options->glyph_name_prefix, j);
			} else {
				features[j]->name = sdscatprintf(sdsempty(), "%c%c%c%c_%05d", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
				                                 (tag >> 8) & 0xff, tag & 0xff, j);
			}
			uint32_t featureOffset = featureListOffset + read_16u(data + featureListOffset + 2 + j * 6 + 4);

			checkLength(featureOffset + 4);
			tableid_t lookupCount = read_16u(data + featureOffset + 2);
			checkLength(featureOffset + 4 + lookupCount * 2);
			features[j]->lookupCount = lookupCount;
			NEW(features[j]->lookups, lookupCount);
			for (tableid_t k = 0; k < lookupCount; k++) {
				tableid_t lookupid = read_16u(data + featureOffset + 4 + k * 2);
				if (lookupid < table->lookupCount) {
					features[j]->lookups[k] = table->lookups[lookupid];
					if (!features[j]->lookups[k]->name) {
						if (options->glyph_name_prefix) {
							features[j]->lookups[k]->name = sdscatprintf(
							    sdsempty(), "lookup_%s_%c%c%c%c_%d", options->glyph_name_prefix, (tag >> 24) & 0xFF,
							    (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff, lnk++);
						} else {
							features[j]->lookups[k]->name =
							    sdscatprintf(sdsempty(), "lookup_%c%c%c%c_%d", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
							                 (tag >> 8) & 0xff, tag & 0xff, lnk++);
						}
					}
				}
			}
		}
		table->featureCount = featureCount;
		table->features = features;
	}

	// parse script list
	{
		tableid_t scriptCount = read_16u(data + scriptListOffset);
		checkLength(scriptListOffset + 2 + 6 * scriptCount);

		uint32_t nLanguageCombinations = 0;
		for (tableid_t j = 0; j < scriptCount; j++) {
			uint32_t scriptOffset = scriptListOffset + read_16u(data + scriptListOffset + 2 + 6 * j + 4);
			checkLength(scriptOffset + 4);

			tableid_t defaultLangSystem = read_16u(data + scriptOffset);
			nLanguageCombinations += (defaultLangSystem ? 1 : 0) + read_16u(data + scriptOffset + 2);
		}

		table->languageCount = nLanguageCombinations;
		otl_LanguageSystem **languages;
		NEW(languages, nLanguageCombinations);

		tableid_t currentLang = 0;
		for (tableid_t j = 0; j < scriptCount; j++) {
			uint32_t tag = read_32u(data + scriptListOffset + 2 + 6 * j);
			uint32_t scriptOffset = scriptListOffset + read_16u(data + scriptListOffset + 2 + 6 * j + 4);
			tableid_t defaultLangSystem = read_16u(data + scriptOffset);
			if (defaultLangSystem) {
				NEW(languages[currentLang]);
				languages[currentLang]->name =
				    sdscatprintf(sdsempty(), "%c%c%c%c%cDFLT", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
				                 (tag >> 8) & 0xff, tag & 0xff, SCRIPT_LANGUAGE_SEPARATOR);
				parseLanguage(data, tableLength, scriptOffset + defaultLangSystem, languages[currentLang],
				              table->featureCount, table->features);
				currentLang += 1;
			}
			tableid_t langSysCount = read_16u(data + scriptOffset + 2);
			for (tableid_t k = 0; k < langSysCount; k++) {
				uint32_t langTag = read_32u(data + scriptOffset + 4 + 6 * k);
				tableid_t langSys = read_16u(data + scriptOffset + 4 + 6 * k + 4);
				NEW(languages[currentLang]);
				languages[currentLang]->name =
				    sdscatprintf(sdsempty(), "%c%c%c%c%c%c%c%c%c", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
				                 (tag >> 8) & 0xff, tag & 0xff, SCRIPT_LANGUAGE_SEPARATOR, (langTag >> 24) & 0xFF,
				                 (langTag >> 16) & 0xFF, (langTag >> 8) & 0xff, langTag & 0xff);
				parseLanguage(data, tableLength, scriptOffset + langSys, languages[currentLang], table->featureCount,
				              table->features);
				currentLang += 1;
			}
		}

		table->languages = languages;
	}
	// name all lookups
	for (tableid_t j = 0; j < table->lookupCount; j++) {
		if (!table->lookups[j]->name) {
			if (options->glyph_name_prefix) {
				table->lookups[j]->name = sdscatprintf(sdsempty(), "lookup_%s_%02x_%d", options->glyph_name_prefix,
				                                       table->lookups[j]->type, j);
			} else {
				table->lookups[j]->name = sdscatprintf(sdsempty(), "lookup_%02x_%d", table->lookups[j]->type, j);
			}
		}
	}
	return table;
FAIL:
	if (table) otfcc_deleteOtl(table);
	return NULL;
}

static void otfcc_readOtl_lookup(font_file_pointer data, uint32_t tableLength, otl_Lookup *lookup,
                                 const otfcc_Options *options) {
	lookup->flags = read_16u(data + lookup->_offset + 2);
	lookup->subtableCount = read_16u(data + lookup->_offset + 4);
	if (!lookup->subtableCount || tableLength < lookup->_offset + 6 + 2 * lookup->subtableCount) {
		lookup->type = otl_type_unknown;
		lookup->subtableCount = 0;
		lookup->subtables = NULL;
		return;
	}
	NEW(lookup->subtables, lookup->subtableCount);
	for (tableid_t j = 0; j < lookup->subtableCount; j++) {
		uint32_t subtableOffset = lookup->_offset + read_16u(data + lookup->_offset + 6 + j * 2);
		lookup->subtables[j] = otfcc_readOtl_subtable(data, tableLength, subtableOffset, lookup->type, options);
	}
	if (lookup->type == otl_type_gsub_extend || lookup->type == otl_type_gpos_extend) {
		lookup->type = 0;
		for (tableid_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) {
				lookup->type = lookup->subtables[j]->extend.type;
				break;
			}
		}
		if (lookup->type) {
			for (tableid_t j = 0; j < lookup->subtableCount; j++) {
				if (lookup->subtables[j] && lookup->subtables[j]->extend.type == lookup->type) {
					// this subtable is valid
					otl_Subtable *st = lookup->subtables[j]->extend.subtable;
					FREE(lookup->subtables[j]);
					lookup->subtables[j] = st;
				} else if (lookup->subtables[j]) {
					// delete this subtable
					otl_Lookup *temp;
					NEW(temp);
					temp->type = lookup->subtables[j]->extend.type;
					temp->subtableCount = 1;
					NEW(temp->subtables, 1);
					temp->subtables[0] = lookup->subtables[j]->extend.subtable;
					DELETE(otfcc_delete_lookup, temp);
					FREE(lookup->subtables[j]);
				}
			}
		} else {
			FREE(lookup->subtables);
			lookup->subtableCount = 0;
			return;
		}
	}
	if (lookup->type == otl_type_gsub_context) lookup->type = otl_type_gsub_chaining;
	if (lookup->type == otl_type_gpos_context) lookup->type = otl_type_gpos_chaining;
}

table_OTL *otfcc_readOtl(otfcc_Packet packet, const otfcc_Options *options, uint32_t tag) {
	table_OTL *otl = NULL;
	FOR_TABLE(tag, table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		otl = otfcc_readOtl_common(
		    data, length,
		    (tag == 'GSUB' ? otl_type_gsub_unknown : tag == 'GPOS' ? otl_type_gpos_unknown : otl_type_unknown),
		    options);
		if (!otl) goto FAIL;
		for (tableid_t j = 0; j < otl->lookupCount; j++) {
			otfcc_readOtl_lookup(data, length, otl->lookups[j], options);
		}
		return otl;
	FAIL:
		if (otl) otfcc_deleteOtl(otl);
		otl = NULL;
	}
	return NULL;
}
