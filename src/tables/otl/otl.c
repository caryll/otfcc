#include "otl.h"

static const char SCRIPT_LANGUAGE_SEPARATOR = '_';

typedef struct {
	char *name;
	otl_lookup *lookup;
	UT_hash_handle hh;
} lookup_hash;

typedef struct {
	char *name;
	otl_feature *feature;
	UT_hash_handle hh;
} feature_hash;

typedef struct {
	char *name;
	otl_language_system *script;
	UT_hash_handle hh;
} language_hash;

void caryll_delete_lookup(otl_lookup *lookup);
otl_subtable *caryll_read_otl_subtable(font_file_pointer data, uint32_t tableLength,
                                       uint32_t subtableOffset, otl_lookup_type lookupType);
static INLINE void _lookup_to_json(otl_lookup *lookup, json_value *dump);
static INLINE bool _parse_lookup(json_value *lookup, char *lookupName, lookup_hash **lh);
static INLINE bool _write_subtable(otl_lookup *lookup, caryll_buffer *buf,
                                   uint32_t **subtableOffsets, uint32_t *lastOffset);

// COMMON PART
table_otl *caryll_new_otl() {
	table_otl *table;
	NEW(table);
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
			if (table->languages[j]->name) sdsfree(table->languages[j]->name);
			if (table->languages[j]->features) free(table->languages[j]->features);
			free(table->languages[j]);
		}
		free(table->languages);
	}
	if (table->features) {
		for (uint16_t j = 0; j < table->featureCount; j++) {
			if (table->features[j]->name) sdsfree(table->features[j]->name);
			if (table->features[j]->lookups) free(table->features[j]->lookups);
			free(table->features[j]);
		}
		free(table->features);
	}
	if (table->lookups) {
		for (uint16_t j = 0; j < table->lookupCount; j++) {
			caryll_delete_lookup(table->lookups[j]);
		}
		free(table->lookups);
	}
	free(table);
}

static INLINE void parseLanguage(font_file_pointer data, uint32_t tableLength, uint32_t base,
                                 otl_language_system *lang, uint16_t featureCount,
                                 otl_feature **features) {
	checkLength(base + 6);
	uint16_t rid = read_16u(data + base + 2);
	if (rid < featureCount) {
		lang->requiredFeature = features[rid];
	} else {
		lang->requiredFeature = NULL;
	}
	lang->featureCount = read_16u(data + base + 4);
	checkLength(base + 6 + lang->featureCount * 2);

	NEW_N(lang->features, lang->featureCount);
	for (uint16_t j = 0; j < lang->featureCount; j++) {
		uint16_t featureIndex = read_16u(data + base + 6 + 2 * j);
		if (featureIndex < featureCount) {
			lang->features[j] = features[featureIndex];
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

static INLINE table_otl *caryll_read_otl_common(font_file_pointer data, uint32_t tableLength,
                                                otl_lookup_type lookup_type_base) {
	table_otl *table = caryll_new_otl();
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
		uint16_t lookupCount = read_16u(data + lookupListOffset);
		checkLength(lookupListOffset + 2 + lookupCount * 2);
		otl_lookup **lookups;
		NEW_N(lookups, lookupCount);
		for (uint16_t j = 0; j < lookupCount; j++) {
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
		uint16_t featureCount = read_16u(data + featureListOffset);
		checkLength(featureListOffset + 2 + featureCount * 6);
		otl_feature **features;
		NEW_N(features, featureCount);
		uint16_t lnk = 0;
		for (uint16_t j = 0; j < featureCount; j++) {
			otl_feature *feature;
			NEW(feature);
			features[j] = feature;
			uint32_t tag = read_32u(data + featureListOffset + 2 + j * 6);
			features[j]->name = sdscatprintf(sdsempty(), "%c%c%c%c_%d", (tag >> 24) & 0xFF,
			                                 (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff, j);
			uint32_t featureOffset =
			    featureListOffset + read_16u(data + featureListOffset + 2 + j * 6 + 4);

			checkLength(featureOffset + 4);
			uint16_t lookupCount = read_16u(data + featureOffset + 2);
			checkLength(featureOffset + 4 + lookupCount * 2);
			features[j]->lookupCount = lookupCount;
			NEW_N(features[j]->lookups, lookupCount);
			for (uint16_t k = 0; k < lookupCount; k++) {
				uint16_t lookupid = read_16u(data + featureOffset + 4 + k * 2);
				if (lookupid < table->lookupCount) {
					features[j]->lookups[k] = table->lookups[lookupid];
					if (!features[j]->lookups[k]->name) {
						features[j]->lookups[k]->name =
						    sdscatprintf(sdsempty(), "lookup_%c%c%c%c_%d", (tag >> 24) & 0xFF,
						                 (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff, lnk++);
					}
				}
			}
		}
		table->featureCount = featureCount;
		table->features = features;
	}

	// parse script list
	{
		uint16_t scriptCount = read_16u(data + scriptListOffset);
		checkLength(scriptListOffset + 2 + 6 * scriptCount);

		uint32_t nLanguageCombinations = 0;
		for (uint16_t j = 0; j < scriptCount; j++) {
			uint32_t scriptOffset =
			    scriptListOffset + read_16u(data + scriptListOffset + 2 + 6 * j + 4);
			checkLength(scriptOffset + 4);

			uint16_t defaultLangSystem = read_16u(data + scriptOffset);
			nLanguageCombinations +=
			    (defaultLangSystem ? 1 : 0) + read_16u(data + scriptOffset + 2);
		}

		table->languageCount = nLanguageCombinations;
		otl_language_system **languages;
		NEW_N(languages, nLanguageCombinations);

		uint16_t currentLang = 0;
		for (uint16_t j = 0; j < scriptCount; j++) {
			uint32_t tag = read_32u(data + scriptListOffset + 2 + 6 * j);
			uint32_t scriptOffset =
			    scriptListOffset + read_16u(data + scriptListOffset + 2 + 6 * j + 4);
			uint16_t defaultLangSystem = read_16u(data + scriptOffset);
			if (defaultLangSystem) {
				NEW(languages[currentLang]);
				languages[currentLang]->name = sdscatprintf(
				    sdsempty(), "%c%c%c%c%cDFLT", (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
				    (tag >> 8) & 0xff, tag & 0xff, SCRIPT_LANGUAGE_SEPARATOR);
				parseLanguage(data, tableLength, scriptOffset + defaultLangSystem,
				              languages[currentLang], table->featureCount, table->features);
				currentLang += 1;
			}
			uint16_t langSysCount = read_16u(data + scriptOffset + 2);
			for (uint16_t k = 0; k < langSysCount; k++) {
				uint32_t langTag = read_32u(data + scriptOffset + 4 + 6 * k);
				uint16_t langSys = read_16u(data + scriptOffset + 4 + 6 * k + 4);
				NEW(languages[currentLang]);
				languages[currentLang]->name =
				    sdscatprintf(sdsempty(), "%c%c%c%c%c%c%c%c%c", (tag >> 24) & 0xFF,
				                 (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff,
				                 SCRIPT_LANGUAGE_SEPARATOR, (langTag >> 24) & 0xFF,
				                 (langTag >> 16) & 0xFF, (langTag >> 8) & 0xff, langTag & 0xff);
				parseLanguage(data, tableLength, scriptOffset + langSys, languages[currentLang],
				              table->featureCount, table->features);
				currentLang += 1;
			}
		}

		table->languages = languages;
	}
	// name all lookups
	for (uint16_t j = 0; j < table->lookupCount; j++) {
		if (!table->lookups[j]->name)
			table->lookups[j]->name =
			    sdscatprintf(sdsempty(), "lookup_%02x_%d", table->lookups[j]->type, j);
	}
	return table;
FAIL:
	if (table) caryll_delete_otl(table);
	return NULL;
}

static INLINE void caryll_read_otl_lookup(font_file_pointer data, uint32_t tableLength,
                                          otl_lookup *lookup) {
	lookup->flags = read_16u(data + lookup->_offset + 2);
	lookup->subtableCount = read_16u(data + lookup->_offset + 4);
	if (!lookup->subtableCount || tableLength < lookup->_offset + 6 + 2 * lookup->subtableCount) {
		lookup->type = otl_type_unknown;
		lookup->subtableCount = 0;
		lookup->subtables = NULL;
		return;
	}
	NEW_N(lookup->subtables, lookup->subtableCount);
	for (uint16_t j = 0; j < lookup->subtableCount; j++) {
		uint32_t subtableOffset = lookup->_offset + read_16u(data + lookup->_offset + 6 + j * 2);
		lookup->subtables[j] =
		    caryll_read_otl_subtable(data, tableLength, subtableOffset, lookup->type);
	}
	if (lookup->type == otl_type_gsub_extend || lookup->type == otl_type_gpos_extend) {
		lookup->type = 0;
		for (uint16_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) {
				lookup->type = lookup->subtables[j]->extend.type;
				break;
			}
		}
		if (lookup->type) {
			for (uint16_t j = 0; j < lookup->subtableCount; j++) {
				if (lookup->subtables[j] && lookup->subtables[j]->extend.type == lookup->type) {
					// this subtable is valid
					otl_subtable *st = lookup->subtables[j]->extend.subtable;
					FREE(lookup->subtables[j]);
					lookup->subtables[j] = st;
				} else if (lookup->subtables[j]) {
					// delete this subtable
					otl_lookup *temp;
					NEW(temp);
					temp->type = lookup->subtables[j]->extend.type;
					temp->subtableCount = 1;
					NEW_N(temp->subtables, 1);
					temp->subtables[0] = lookup->subtables[j]->extend.subtable;
					DELETE(caryll_delete_lookup, temp);
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

table_otl *caryll_read_otl(caryll_packet packet, uint32_t tag) {
	table_otl *otl = NULL;
	FOR_TABLE(tag, table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		otl = caryll_read_otl_common(
		    data, length,
		    (tag == 'GSUB' ? otl_type_gsub_unknown : tag == 'GPOS' ? otl_type_gpos_unknown
		                                                           : otl_type_unknown));
		if (!otl) goto FAIL;
		for (uint16_t j = 0; j < otl->lookupCount; j++) {
			caryll_read_otl_lookup(data, length, otl->lookups[j]);
		}
		return otl;
	FAIL:
		if (otl) caryll_delete_otl(otl);
		otl = NULL;
	}
	return NULL;
}

static const char *lookupFlagsLabels[] = {"rightToLeft", "ignoreBases", "ignoreLigatures",
                                          "ignoreMarks", NULL};

static INLINE void _declare_lookup_dumper(otl_lookup_type llt, const char *lt,
                                          json_value *(*dumper)(otl_subtable *st),
                                          otl_lookup *lookup, json_value *dump) {
	if (lookup->type == llt) {
		json_object_push(dump, "type", json_string_new(lt));
		json_object_push(dump, "flags", caryll_flags_to_json(lookup->flags, lookupFlagsLabels));
		json_value *subtables = json_array_new(lookup->subtableCount);
		for (uint16_t j = 0; j < lookup->subtableCount; j++)
			if (lookup->subtables[j]) { json_array_push(subtables, dumper(lookup->subtables[j])); }
		json_object_push(dump, "subtables", subtables);
	}
}

void caryll_otl_to_json(table_otl *table, json_value *root, caryll_dump_options *dumpopts,
                        const char *tag) {
	if (!table || !table->languages || !table->lookups || !table->features) return;
	json_value *otl = json_object_new(3);
	{
		// dump script list
		json_value *languages = json_object_new(table->languageCount);
		for (uint16_t j = 0; j < table->languageCount; j++) {
			json_value *language = json_object_new(5);
			if (table->languages[j]->requiredFeature) {
				json_object_push(language, "requiredFeature",
				                 json_string_new(table->languages[j]->requiredFeature->name));
			}
			json_value *features = json_array_new(table->languages[j]->featureCount);
			for (uint16_t k = 0; k < table->languages[j]->featureCount; k++)
				if (table->languages[j]->features[k]) {
					json_array_push(features,
					                json_string_new(table->languages[j]->features[k]->name));
				}
			json_object_push(language, "features", preserialize(features));
			json_object_push(languages, table->languages[j]->name, language);
		}
		json_object_push(otl, "languages", languages);
	}
	{
		// dump feature list
		json_value *features = json_object_new(table->featureCount);
		for (uint16_t j = 0; j < table->featureCount; j++) {
			json_value *feature = json_array_new(table->features[j]->lookupCount);
			for (uint16_t k = 0; k < table->features[j]->lookupCount; k++)
				if (table->features[j]->lookups[k]) {
					json_array_push(feature, json_string_new(table->features[j]->lookups[k]->name));
				}
			json_object_push(features, table->features[j]->name, preserialize(feature));
		}
		json_object_push(otl, "features", features);
	}
	{
		// dump lookups
		json_value *lookups = json_object_new(table->lookupCount);
		for (uint16_t j = 0; j < table->lookupCount; j++) {
			json_value *lookup = json_object_new(5);
			_lookup_to_json(table->lookups[j], lookup);
			json_object_push(lookups, table->lookups[j]->name, lookup);
		}
		json_object_push(otl, "lookups", lookups);
	}
	json_object_push(root, tag, otl);
}

static INLINE bool _declareLookupParser(const char *lt, otl_lookup_type llt,
                                        otl_subtable *(*parser)(json_value *), json_value *_lookup,
                                        char *lookupName, lookup_hash **lh) {
	json_value *type = json_obj_get_type(_lookup, "type", json_string);
	if (!type || strcmp(type->u.string.ptr, lt)) return false;
	lookup_hash *item = NULL;
	HASH_FIND_STR(*lh, lookupName, item);
	if (item) return false;
	json_value *_subtables = json_obj_get_type(_lookup, "subtables", json_array);
	if (!_subtables) return false;

	otl_lookup *lookup;
	NEW(lookup);
	lookup->type = llt;
	lookup->flags = caryll_flags_from_json(json_obj_get(_lookup, "flags"), lookupFlagsLabels);
	lookup->subtableCount = _subtables->u.array.length;
	NEW_N(lookup->subtables, lookup->subtableCount);
	uint16_t jj = 0;
	for (uint16_t j = 0; j < lookup->subtableCount; j++) {
		json_value *_subtable = _subtables->u.array.values[j];
		if (_subtable && _subtable->type == json_object) {
			otl_subtable *_st = parser(_subtable);
			if (_st) { lookup->subtables[jj++] = _st; }
		}
	}
	lookup->subtableCount = jj;

	NEW(item);
	item->name = sdsnew(lookupName);
	lookup->name = sdsdup(item->name);
	item->lookup = lookup;
	item->lookup->name = sdsdup(item->name);
	HASH_ADD_STR(*lh, name, item);

	return true;
}

static INLINE feature_hash *figureOutFeaturesFromJSON(json_value *features, lookup_hash *lh,
                                                      const char *tag) {
	feature_hash *fh = NULL;
	for (uint32_t j = 0; j < features->u.object.length; j++) {
		char *featureName = features->u.object.values[j].name;
		json_value *_feature = features->u.object.values[j].value;
		if (_feature->type == json_array) {
			uint16_t nal = 0;
			otl_lookup **al;
			NEW_N(al, _feature->u.array.length);
			for (uint16_t k = 0; k < _feature->u.array.length; k++) {
				json_value *term = _feature->u.array.values[k];
				if (term->type == json_string) {
					lookup_hash *item = NULL;
					HASH_FIND_STR(lh, term->u.string.ptr, item);
					if (item) { al[nal++] = item->lookup; }
				}
			}
			if (nal > 0) {
				feature_hash *s = NULL;
				HASH_FIND_STR(fh, featureName, s);
				if (!s) {
					NEW(s);
					s->name = sdsnew(featureName);
					NEW(s->feature);
					s->feature->name = sdsdup(s->name);
					s->feature->lookupCount = nal;
					s->feature->lookups = al;
					HASH_ADD_STR(fh, name, s);
				} else {
					fprintf(stderr, "[OTFCC-fea] There is no valid lookup "
					                "assignments for [%s/%s]. This feature "
					                "will be ignored.\n",
					        tag, featureName);
					FREE(al);
				}
			} else {
				fprintf(stderr, "[OTFCC-fea] There is no valid lookup "
				                "assignments for [%s/%s]. This feature will be "
				                "ignored.\n",
				        tag, featureName);
				FREE(al);
			}
		}
	}
	return fh;
}
bool isValidLanguageName(const char *name, const size_t length) {
	return length == 9 && name[4] == SCRIPT_LANGUAGE_SEPARATOR;
}
static INLINE language_hash *figureOutLanguagesFromJson(json_value *languages, feature_hash *fh,
                                                        const char *tag) {
	language_hash *sh = NULL;
	// languages
	for (uint32_t j = 0; j < languages->u.object.length; j++) {
		char *languageName = languages->u.object.values[j].name;
		size_t languageNameLen = languages->u.object.values[j].name_length;
		json_value *_language = languages->u.object.values[j].value;
		if (isValidLanguageName(languageName, languageNameLen) && _language->type == json_object) {
			otl_feature *requiredFeature = NULL;
			json_value *_rf = json_obj_get_type(_language, "requiredFeature", json_string);
			if (_rf) {
				// required feature term
				feature_hash *rf = NULL;
				HASH_FIND_STR(fh, _rf->u.string.ptr, rf);
				if (rf) { requiredFeature = rf->feature; }
			}

			uint16_t naf = 0;
			otl_feature **af = NULL;
			json_value *_features = json_obj_get_type(_language, "features", json_array);
			if (_features) {
				NEW_N(af, _features->u.array.length);
				for (uint16_t k = 0; k < _features->u.array.length; k++) {
					json_value *term = _features->u.array.values[k];
					if (term->type == json_string) {
						feature_hash *item = NULL;
						HASH_FIND_STR(fh, term->u.string.ptr, item);
						if (item) { af[naf++] = item->feature; }
					}
				}
			}
			if (requiredFeature || (af && naf > 0)) {
				language_hash *s = NULL;
				HASH_FIND_STR(sh, languageName, s);
				if (!s) {
					NEW(s);
					s->name = sdsnew(languageName);
					NEW(s->script);
					s->script->name = sdsdup(s->name);
					s->script->requiredFeature = requiredFeature;
					s->script->featureCount = naf;
					s->script->features = af;
					HASH_ADD_STR(sh, name, s);
				} else {
					fprintf(stderr, "[OTFCC-fea] There is no valid feature "
					                "assignments for [%s/%s]. This language "
					                "term will be ignored.\n",
					        tag, languageName);
					if (af) { FREE(af); }
				}
			} else {
				fprintf(stderr, "[OTFCC-fea] There is no valid feature "
				                "assignments for [%s/%s]. This language term "
				                "will be ignored.\n",
				        tag, languageName);

				if (af) { FREE(af); }
			}
		}
	}
	return sh;
}

static INLINE lookup_hash *figureOutLookupsFromJSON(json_value *lookups) {
	lookup_hash *lh = NULL;
	for (uint32_t j = 0; j < lookups->u.object.length; j++) {
		if (lookups->u.object.values[j].value->type == json_object) {
			char *lookupName = lookups->u.object.values[j].name;
			bool parsed = _parse_lookup(lookups->u.object.values[j].value, lookupName, &lh);
			if (!parsed) {
				fprintf(stderr, "[OTFCC-fea] Ignoring unknown or unsupported lookup %s.\n",
				        lookupName);
			}
		}
	}
	return lh;
}
static INLINE int by_feature_name(feature_hash *a, feature_hash *b) {
	return strcmp(a->name, b->name);
}
static INLINE int by_language_name(language_hash *a, language_hash *b) {
	return strcmp(a->name, b->name);
}
table_otl *caryll_otl_from_json(json_value *root, caryll_dump_options *dumpopts, const char *tag) {
	table_otl *otl = NULL;
	NEW(otl);
	json_value *table = json_obj_get_type(root, tag, json_object);
	if (!table) goto FAIL;
	json_value *languages = json_obj_get_type(table, "languages", json_object);
	json_value *features = json_obj_get_type(table, "features", json_object);
	json_value *lookups = json_obj_get_type(table, "lookups", json_object);
	if (!languages || !features || !lookups) goto FAIL;

	lookup_hash *lh = figureOutLookupsFromJSON(lookups);
	feature_hash *fh = figureOutFeaturesFromJSON(features, lh, tag);
	HASH_SORT(fh, by_feature_name);
	language_hash *sh = figureOutLanguagesFromJson(languages, fh, tag);
	HASH_SORT(sh, by_language_name);
	if (!HASH_COUNT(lh) || !HASH_COUNT(fh) || !HASH_COUNT(sh)) goto FAIL;

	{
		lookup_hash *s, *tmp;
		otl->lookupCount = HASH_COUNT(lh);
		NEW_N(otl->lookups, otl->lookupCount);
		uint16_t j = 0;
		HASH_ITER(hh, lh, s, tmp) {
			otl->lookups[j] = s->lookup;
			HASH_DEL(lh, s);
			sdsfree(s->name);
			free(s);
			j++;
		}
	}
	{
		feature_hash *s, *tmp;
		otl->featureCount = HASH_COUNT(fh);
		NEW_N(otl->features, otl->featureCount);
		uint16_t j = 0;
		HASH_ITER(hh, fh, s, tmp) {
			otl->features[j] = s->feature;
			HASH_DEL(fh, s);
			sdsfree(s->name);
			free(s);
			j++;
		}
	}
	{
		language_hash *s, *tmp;
		otl->languageCount = HASH_COUNT(sh);
		NEW_N(otl->languages, otl->languageCount);
		uint16_t j = 0;
		HASH_ITER(hh, sh, s, tmp) {
			otl->languages[j] = s->script;
			HASH_DEL(sh, s);
			sdsfree(s->name);
			free(s);
			j++;
		}
	}
	return otl;
FAIL:
	fprintf(stderr, "[OTFCC-fea] Ignoring invalid or incomplete OTL table %s.\n", tag);
	caryll_delete_otl(otl);
	return NULL;
}

static bool _declare_lookup_writer(otl_lookup_type type,
                                   caryll_buffer *(*fn)(otl_subtable *_subtable),
                                   otl_lookup *lookup, caryll_buffer *buf,
                                   uint32_t **subtableOffsets, uint32_t *lastOffset) {
	if (lookup->type == type) {
		NEW_N(*subtableOffsets, lookup->subtableCount);
		for (uint16_t j = 0; j < lookup->subtableCount; j++) {
			(*subtableOffsets)[j] = buf->cursor;
			*lastOffset = buf->cursor;
			bufwrite_bufdel(buf, fn(lookup->subtables[j]));
		}
		return true;
	}
	return false;
}

// When writing lookups, otfcc will try to maintain everything correctly.
static INLINE caryll_buffer *writeOTLLookups(table_otl *table) {
	caryll_buffer *bufl = bufnew();
	caryll_buffer *bufsts = bufnew();
	uint32_t **subtableOffsets;
	NEW_N(subtableOffsets, table->lookupCount);
	bool *lookupWritten;
	NEW_N(lookupWritten, table->lookupCount);

	uint32_t lastOffset = 0;
	for (uint16_t j = 0; j < table->lookupCount; j++) {
		subtableOffsets[j] = NULL;
		lookupWritten[j] =
		    _write_subtable(table->lookups[j], bufsts, &(subtableOffsets[j]), &lastOffset);
	}
	// estimate the length of headers
	size_t headerSize = 2 + 2 * table->lookupCount;
	for (uint16_t j = 0; j < table->lookupCount; j++) {
		if (lookupWritten[j]) { headerSize += 6 + 2 * table->lookups[j]->subtableCount; }
	}
	bool useExtended = lastOffset >= 0xFF00 - headerSize;
	if (useExtended) {
		fprintf(stderr, "[OTFCC-fea] Using extended OpenType table layout.\n");
		for (uint16_t j = 0; j < table->lookupCount; j++) {
			if (lookupWritten[j]) { headerSize += 8 * table->lookups[j]->subtableCount; }
		}
	}
	// write the header
	bufwrite16b(bufl, table->lookupCount);
	size_t hp = bufl->cursor;
	bufseek(bufl, 2 + table->lookupCount * 2);
	for (uint16_t j = 0; j < table->lookupCount; j++) {
		if (!lookupWritten[j]) {
			fprintf(stderr, "Lookup %s not written.\n", table->lookups[j]->name);
			continue;
		}

		otl_lookup *lookup = table->lookups[j];
		size_t lookupOffset = bufl->cursor;
		if (lookupOffset > 0xFFFF) {
			fprintf(stderr, "[OTFCC-fea] Warning, Lookup %s Written at 0x%" PRIx32 ", "
			                "this lookup may be corrupted.\n",
			        table->lookups[j]->name, (uint32_t)lookupOffset);
		}
		// lookup type
		if (useExtended) {
			bufwrite16b(bufl, (lookup->type > otl_type_gpos_unknown
			                       ? otl_type_gpos_extend - otl_type_gpos_unknown
			                       : lookup->type > otl_type_gsub_unknown
			                             ? otl_type_gsub_extend - otl_type_gsub_unknown
			                             : 0));
		} else {
			bufwrite16b(bufl, (lookup->type > otl_type_gpos_unknown
			                       ? lookup->type - otl_type_gpos_unknown
			                       : lookup->type > otl_type_gsub_unknown
			                             ? lookup->type - otl_type_gsub_unknown
			                             : 0));
		}
		// lookup flags
		bufwrite16b(bufl, lookup->flags);
		bufwrite16b(bufl, lookup->subtableCount);
		if (useExtended) {
			for (uint16_t k = 0; k < lookup->subtableCount; k++) { // subtable offsets
				bufwrite16b(bufl, 6 + 2 * lookup->subtableCount + 8 * k);
			}
			for (uint16_t k = 0; k < lookup->subtableCount; k++) { // extension subtables
				size_t subtableStart = bufl->cursor;
				bufwrite16b(bufl, 1);
				bufwrite16b(bufl, (lookup->type > otl_type_gpos_unknown
				                       ? lookup->type - otl_type_gpos_unknown
				                       : lookup->type > otl_type_gsub_unknown
				                             ? lookup->type - otl_type_gsub_unknown
				                             : 0));
				bufwrite32b(bufl, subtableOffsets[j][k] + headerSize - subtableStart);
			}
		} else {
			for (uint16_t k = 0; k < lookup->subtableCount; k++) { // subtable offsets
				bufwrite16b(bufl, subtableOffsets[j][k] + headerSize - lookupOffset);
			}
		}
		free(subtableOffsets[j]);
		size_t cp = bufl->cursor;

		bufseek(bufl, hp);
		bufwrite16b(bufl, lookupOffset);
		hp = bufl->cursor;
		bufseek(bufl, cp);
	}
	free(subtableOffsets);
	free(lookupWritten);
	bufseek(bufl, headerSize);
	bufwrite_bufdel(bufl, bufsts);
	return bufl;
}

static uint32_t featureNameToTag(sds name) {
	uint32_t tag = 0;
	if (sdslen(name) > 0) { tag |= ((uint8_t)name[0]) << 24; }
	if (sdslen(name) > 1) { tag |= ((uint8_t)name[1]) << 16; }
	if (sdslen(name) > 2) { tag |= ((uint8_t)name[2]) << 8; }
	if (sdslen(name) > 3) { tag |= ((uint8_t)name[3]) << 0; }
	return tag;
}
static INLINE caryll_buffer *writeOTLFeatures(table_otl *table) {
	caryll_buffer *buff = bufnew();
	bufwrite16b(buff, table->featureCount);
	size_t offset = 2 + table->featureCount * 6;
	for (uint16_t j = 0; j < table->featureCount; j++) {
		bufwrite32b(buff, featureNameToTag(table->features[j]->name));
		bufwrite16b(buff, offset);
		size_t cp = buff->cursor;
		bufseek(buff, offset);
		bufwrite16b(buff, 0);
		bufwrite16b(buff, table->features[j]->lookupCount);
		for (uint16_t k = 0; k < table->features[j]->lookupCount; k++) {
			// reverse lookup
			for (uint16_t l = 0; l < table->lookupCount; l++) {
				if (table->features[j]->lookups[k] == table->lookups[l]) {
					bufwrite16b(buff, l);
					break;
				}
			}
		}
		offset = buff->cursor;
		bufseek(buff, cp);
	}
	return buff;
}

typedef struct {
	sds tag;
	uint16_t lc;
	otl_language_system *dl;
	otl_language_system **ll;
	UT_hash_handle hh;
} script_stat_hash;

static INLINE caryll_buffer *writeLanguage(otl_language_system *lang, table_otl *table) {
	caryll_buffer *buf = bufnew();
	bufwrite16b(buf, 0x0000);
	if (lang->requiredFeature) {
		bool found = false;
		for (uint16_t j = 0; j < table->featureCount; j++)
			if (table->features[j] == lang->requiredFeature) {
				bufwrite16b(buf, j);
				found = true;
				break;
			}
		if (!found) bufwrite16b(buf, 0xFFFF);
	} else {
		bufwrite16b(buf, 0xFFFF);
	}
	bufwrite16b(buf, lang->featureCount);
	for (uint16_t k = 0; k < lang->featureCount; k++) {
		bool found = false;
		for (uint16_t j = 0; j < table->featureCount; j++)
			if (table->features[j] == lang->features[k]) {
				bufwrite16b(buf, j);
				found = true;
				break;
			}
		if (!found) bufwrite16b(buf, 0xFFFF);
	}
	return buf;
}

static INLINE caryll_buffer *writeScript(script_stat_hash *script, table_otl *table) {
	caryll_buffer *buf = bufnew();
	size_t offset = script->lc * 6 + 4;
	if (script->dl) {
		bufwrite16b(buf, offset);
		size_t cp = buf->cursor;
		bufseek(buf, offset);
		bufwrite_bufdel(buf, writeLanguage(script->dl, table));
		offset = buf->cursor;
		bufseek(buf, cp);
	} else {
		bufwrite16b(buf, 0);
	}
	bufwrite16b(buf, script->lc);
	for (uint16_t j = 0; j < script->lc; j++) {
		sds tag = sdsnewlen(script->ll[j]->name + 5, 4);
		bufwrite32b(buf, featureNameToTag(tag));
		sdsfree(tag);
		bufwrite16b(buf, offset);
		size_t cp = buf->cursor;
		bufseek(buf, offset);
		bufwrite_bufdel(buf, writeLanguage(script->ll[j], table));
		offset = buf->cursor;
		bufseek(buf, cp);
	}
	return buf;
}
static INLINE caryll_buffer *writeOTLScriptAndLanguages(table_otl *table) {
	caryll_buffer *bufs = bufnew();
	script_stat_hash *h = NULL;
	for (uint16_t j = 0; j < table->languageCount; j++) {
		sds scriptTag = sdsnewlen(table->languages[j]->name, 4);
		bool isDefault = strncmp(table->languages[j]->name + 5, "DFLT", 4) == 0 ||
		                 strncmp(table->languages[j]->name + 5, "dflt", 4) == 0;
		script_stat_hash *s = NULL;
		HASH_FIND_STR(h, scriptTag, s);
		if (s) {
			if (isDefault) {
				s->dl = table->languages[j];
			} else {
				s->lc += 1;
				s->ll[s->lc - 1] = table->languages[j];
			}
		} else {
			NEW(s);
			s->tag = scriptTag;
			s->dl = NULL;
			NEW_N(s->ll, table->languageCount);
			if (isDefault) {
				s->dl = table->languages[j];
				s->lc = 0;
			} else {
				s->lc = 1;
				s->ll[s->lc - 1] = table->languages[j];
			}
			HASH_ADD_STR(h, tag, s);
		}
	}
	bufwrite16b(bufs, HASH_COUNT(h));
	size_t offset = 2 + 6 * HASH_COUNT(h);
	script_stat_hash *s, *tmp;
	HASH_ITER(hh, h, s, tmp) {
		bufwrite32b(bufs, featureNameToTag(s->tag));
		bufwrite16b(bufs, offset);
		size_t cp = bufs->cursor;
		bufseek(bufs, offset);
		bufwrite_bufdel(bufs, writeScript(s, table));
		offset = bufs->cursor;
		bufseek(bufs, cp);

		HASH_DEL(h, s);
		sdsfree(s->tag);
		free(s->ll);
		free(s);
	}
	return bufs;
}

caryll_buffer *caryll_write_otl(table_otl *table) {
	caryll_buffer *buf = bufnew();
	bufwrite32b(buf, 0x10000);

	caryll_buffer *bufl = writeOTLLookups(table);
	caryll_buffer *buff = writeOTLFeatures(table);
	caryll_buffer *bufs = writeOTLScriptAndLanguages(table);

	size_t rootOffset = 10;
	{
		bufwrite16b(buf, rootOffset);
		size_t cp = buf->cursor;
		bufseek(buf, rootOffset);
		bufwrite_bufdel(buf, bufs);
		rootOffset = buf->cursor;
		bufseek(buf, cp);
	}
	{
		bufwrite16b(buf, rootOffset);
		size_t cp = buf->cursor;
		bufseek(buf, rootOffset);
		bufwrite_bufdel(buf, buff);
		rootOffset = buf->cursor;
		bufseek(buf, cp);
	}
	{
		bufwrite16b(buf, rootOffset);
		size_t cp = buf->cursor;
		bufseek(buf, rootOffset);
		bufwrite_bufdel(buf, bufl);
		rootOffset = buf->cursor;
		bufseek(buf, cp);
	}
	return buf;
}

////////////////////////////////////////////////////////////////////////////////////////
//                                    CONFIG PART //
////////////////////////////////////////////////////////////////////////////////////////

#define DELETE_TYPE(type, fn)                                                                      \
	case type:                                                                                     \
		fn(lookup);                                                                                \
		break;
#define LOOKUP_READER(llt, fn)                                                                     \
	case llt:                                                                                      \
		return fn(data, tableLength, subtableOffset);
#define LOOKUP_DUMPER(llt, fn) _declare_lookup_dumper(llt, tableNames[llt], fn, lookup, dump);
#define LOOKUP_PARSER(llt, parser)                                                                 \
	if (!parsed) {                                                                                 \
		parsed = _declareLookupParser(tableNames[llt], llt, parser, lookup, lookupName, lh);       \
	}
#define LOOKUP_WRITER(type, fn)                                                                    \
	if (!written)                                                                                  \
		written = _declare_lookup_writer(type, fn, lookup, buf, subtableOffsets, lastOffset);
static const char *tableNames[] = {[otl_type_unknown] = "unknown",
                                   [otl_type_gsub_unknown] = "gsub_unknown",
                                   [otl_type_gsub_single] = "gsub_single",
                                   [otl_type_gsub_multiple] = "gsub_multiple",
                                   [otl_type_gsub_alternate] = "gsub_alternate",
                                   [otl_type_gsub_ligature] = "gsub_ligature",
                                   [otl_type_gsub_context] = "gsub_context",
                                   [otl_type_gsub_chaining] = "gsub_chaining",
                                   [otl_type_gsub_extend] = "gsub_extend",
                                   [otl_type_gsub_reverse] = "gsub_reverse",
                                   [otl_type_gpos_unknown] = "gpos_unknown",
                                   [otl_type_gpos_single] = "gpos_single",
                                   [otl_type_gpos_pair] = "gpos_pair",
                                   [otl_type_gpos_cursive] = "gpos_cursive",
                                   [otl_type_gpos_mark_to_base] = "gpos_mark_to_base",
                                   [otl_type_gpos_mark_to_ligature] = "gpos_mark_to_ligature",
                                   [otl_type_gpos_mark_to_mark] = "gpos_mark_to_mark",
                                   [otl_type_gpos_context] = "gpos_context",
                                   [otl_type_gpos_chaining] = "gpos_chaining",
                                   [otl_type_gpos_extend] = "gpos_extend"};

void caryll_delete_lookup(otl_lookup *lookup) {
	if (!lookup) return;
	switch (lookup->type) {
		DELETE_TYPE(otl_type_gsub_single, caryll_delete_gsub_single);
		DELETE_TYPE(otl_type_gsub_multiple, caryll_delete_gsub_multi);
		DELETE_TYPE(otl_type_gsub_alternate, caryll_delete_gsub_multi);
		DELETE_TYPE(otl_type_gsub_ligature, caryll_delete_gsub_ligature);
		DELETE_TYPE(otl_type_gsub_chaining, caryll_delete_chaining);
		DELETE_TYPE(otl_type_gsub_reverse, caryll_delete_gsub_reverse);
		DELETE_TYPE(otl_type_gpos_single, caryll_delete_gpos_single);
		DELETE_TYPE(otl_type_gpos_pair, caryll_delete_gpos_pair);
		DELETE_TYPE(otl_type_gpos_cursive, caryll_delete_gpos_cursive);
		DELETE_TYPE(otl_type_gpos_chaining, caryll_delete_chaining);
		DELETE_TYPE(otl_type_gpos_mark_to_base, caryll_delete_gpos_mark_to_single);
		DELETE_TYPE(otl_type_gpos_mark_to_mark, caryll_delete_gpos_mark_to_single);
		DELETE_TYPE(otl_type_gpos_mark_to_ligature, caryll_delete_gpos_mark_to_ligature);
		default:
			free(lookup);
			break;
	}
}

otl_subtable *caryll_read_otl_subtable(font_file_pointer data, uint32_t tableLength,
                                       uint32_t subtableOffset, otl_lookup_type lookupType) {
	switch (lookupType) {
		LOOKUP_READER(otl_type_gsub_single, caryll_read_gsub_single);
		LOOKUP_READER(otl_type_gsub_multiple, caryll_read_gsub_multi);
		LOOKUP_READER(otl_type_gsub_alternate, caryll_read_gsub_multi);
		LOOKUP_READER(otl_type_gsub_ligature, caryll_read_gsub_ligature);
		LOOKUP_READER(otl_type_gsub_chaining, caryll_read_chaining);
		LOOKUP_READER(otl_type_gsub_reverse, caryll_read_gsub_reverse);
		LOOKUP_READER(otl_type_gpos_chaining, caryll_read_chaining);
		LOOKUP_READER(otl_type_gsub_context, caryll_read_contextual);
		LOOKUP_READER(otl_type_gpos_context, caryll_read_contextual);
		LOOKUP_READER(otl_type_gpos_single, caryll_read_gpos_single);
		LOOKUP_READER(otl_type_gpos_pair, caryll_read_gpos_pair);
		LOOKUP_READER(otl_type_gpos_cursive, caryll_read_gpos_cursive);
		LOOKUP_READER(otl_type_gpos_mark_to_base, caryll_read_gpos_mark_to_single);
		LOOKUP_READER(otl_type_gpos_mark_to_mark, caryll_read_gpos_mark_to_single);
		LOOKUP_READER(otl_type_gpos_mark_to_ligature, caryll_read_gpos_mark_to_ligature);
		LOOKUP_READER(otl_type_gsub_extend, caryll_read_otl_gsub_extend);
		LOOKUP_READER(otl_type_gpos_extend, caryll_read_otl_gpos_extend);
		default:
			return NULL;
	}
}

static INLINE void _lookup_to_json(otl_lookup *lookup, json_value *dump) {
	LOOKUP_DUMPER(otl_type_gsub_single, caryll_gsub_single_to_json);
	LOOKUP_DUMPER(otl_type_gsub_multiple, caryll_gsub_multi_to_json);
	LOOKUP_DUMPER(otl_type_gsub_alternate, caryll_gsub_multi_to_json);
	LOOKUP_DUMPER(otl_type_gsub_ligature, caryll_gsub_ligature_to_json);
	LOOKUP_DUMPER(otl_type_gsub_chaining, caryll_chaining_to_json);
	LOOKUP_DUMPER(otl_type_gsub_reverse, caryll_gsub_reverse_to_json);
	LOOKUP_DUMPER(otl_type_gpos_chaining, caryll_chaining_to_json);
	LOOKUP_DUMPER(otl_type_gpos_single, caryll_gpos_single_to_json);
	LOOKUP_DUMPER(otl_type_gpos_pair, caryll_gpos_pair_to_json);
	LOOKUP_DUMPER(otl_type_gpos_cursive, caryll_gpos_cursive_to_json);
	LOOKUP_DUMPER(otl_type_gpos_mark_to_base, caryll_gpos_mark_to_single_to_json);
	LOOKUP_DUMPER(otl_type_gpos_mark_to_mark, caryll_gpos_mark_to_single_to_json);
	LOOKUP_DUMPER(otl_type_gpos_mark_to_ligature, caryll_gpos_mark_to_ligature_to_json);
}

static INLINE bool _parse_lookup(json_value *lookup, char *lookupName, lookup_hash **lh) {
	bool parsed = false;
	LOOKUP_PARSER(otl_type_gsub_single, caryll_gsub_single_from_json);
	LOOKUP_PARSER(otl_type_gsub_multiple, caryll_gsub_multi_from_json);
	LOOKUP_PARSER(otl_type_gsub_alternate, caryll_gsub_multi_from_json);
	LOOKUP_PARSER(otl_type_gsub_ligature, caryll_gsub_ligature_from_json);
	LOOKUP_PARSER(otl_type_gsub_chaining, caryll_chaining_from_json);
	LOOKUP_PARSER(otl_type_gsub_reverse, caryll_gsub_reverse_from_json);
	LOOKUP_PARSER(otl_type_gpos_single, caryll_gpos_single_from_json);
	LOOKUP_PARSER(otl_type_gpos_pair, caryll_gpos_pair_from_json);
	LOOKUP_PARSER(otl_type_gpos_cursive, caryll_gpos_cursive_from_json);
	LOOKUP_PARSER(otl_type_gpos_chaining, caryll_chaining_from_json);
	LOOKUP_PARSER(otl_type_gpos_mark_to_base, caryll_gpos_mark_to_single_from_json);
	LOOKUP_PARSER(otl_type_gpos_mark_to_mark, caryll_gpos_mark_to_single_from_json);
	LOOKUP_PARSER(otl_type_gpos_mark_to_ligature, caryll_gpos_mark_to_ligature_from_json);
	return parsed;
}

static INLINE bool _write_subtable(otl_lookup *lookup, caryll_buffer *buf,
                                   uint32_t **subtableOffsets, uint32_t *lastOffset) {
	bool written = false;
	LOOKUP_WRITER(otl_type_gsub_single, caryll_write_gsub_single_subtable);
	LOOKUP_WRITER(otl_type_gsub_multiple, caryll_write_gsub_multi_subtable);
	LOOKUP_WRITER(otl_type_gsub_alternate, caryll_write_gsub_multi_subtable);
	LOOKUP_WRITER(otl_type_gsub_ligature, caryll_write_gsub_ligature_subtable);
	LOOKUP_WRITER(otl_type_gsub_chaining, caryll_write_chaining);
	LOOKUP_WRITER(otl_type_gsub_reverse, caryll_write_gsub_reverse);
	LOOKUP_WRITER(otl_type_gpos_single, caryll_write_gpos_single);
	LOOKUP_WRITER(otl_type_gpos_pair, caryll_write_gpos_pair);
	LOOKUP_WRITER(otl_type_gpos_cursive, caryll_write_gpos_cursive);
	LOOKUP_WRITER(otl_type_gpos_chaining, caryll_write_chaining);
	LOOKUP_WRITER(otl_type_gpos_mark_to_base, caryll_write_gpos_mark_to_single);
	LOOKUP_WRITER(otl_type_gpos_mark_to_mark, caryll_write_gpos_mark_to_single);
	LOOKUP_WRITER(otl_type_gpos_mark_to_ligature, caryll_write_gpos_mark_to_ligature);
	return written;
}
