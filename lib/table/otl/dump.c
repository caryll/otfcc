#include "private.h"

static void _declare_lookup_dumper(otl_LookupType llt, const char *lt, json_value *(*dumper)(const otl_Subtable *st),
                                   otl_Lookup *lookup, json_value *dump) {
	if (lookup->type == llt) {
		json_object_push(dump, "type", json_string_new(lt));
		json_object_push(dump, "flags", otfcc_dump_flags(lookup->flags, lookupFlagsLabels));
		if (lookup->flags >> 8) { json_object_push(dump, "markAttachmentType", json_integer_new(lookup->flags >> 8)); }
		json_value *subtables = json_array_new(lookup->subtableCount);
		for (tableid_t j = 0; j < lookup->subtableCount; j++)
			if (lookup->subtables[j]) { json_array_push(subtables, dumper(lookup->subtables[j])); }
		json_object_push(dump, "subtables", subtables);
	}
}


#define LOOKUP_DUMPER(llt, fn) _declare_lookup_dumper(llt, tableNames[llt], fn, lookup, dump);

static void _dump_lookup(otl_Lookup *lookup, json_value *dump) {
	LOOKUP_DUMPER(otl_type_gsub_single, otl_gsub_dump_single);
	LOOKUP_DUMPER(otl_type_gsub_multiple, otl_gsub_dump_multi);
	LOOKUP_DUMPER(otl_type_gsub_alternate, otl_gsub_dump_multi);
	LOOKUP_DUMPER(otl_type_gsub_ligature, otl_gsub_dump_ligature);
	LOOKUP_DUMPER(otl_type_gsub_chaining, otl_dump_chaining);
	LOOKUP_DUMPER(otl_type_gsub_reverse, otl_gsub_dump_reverse);
	LOOKUP_DUMPER(otl_type_gpos_chaining, otl_dump_chaining);
	LOOKUP_DUMPER(otl_type_gpos_single, otl_gpos_dump_single);
	LOOKUP_DUMPER(otl_type_gpos_pair, otl_gpos_dump_pair);
	LOOKUP_DUMPER(otl_type_gpos_cursive, otl_gpos_dump_cursive);
	LOOKUP_DUMPER(otl_type_gpos_markToBase, otl_gpos_dump_markToSingle);
	LOOKUP_DUMPER(otl_type_gpos_markToMark, otl_gpos_dump_markToSingle);
	LOOKUP_DUMPER(otl_type_gpos_markToLigature, otl_gpos_dump_markToLigature);
}


void otfcc_dumpOtl(const table_OTL *table, json_value *root, const otfcc_Options *options, const char *tag) {
	if (!table || !table->languages || !table->lookups || !table->features) return;
	loggedStep("%s", tag) {
		json_value *otl = json_object_new(3);
		loggedStep("Languages") {
			// dump script list
			json_value *languages = json_object_new(table->languageCount);
			for (tableid_t j = 0; j < table->languageCount; j++) {
				json_value *language = json_object_new(5);
				if (table->languages[j]->requiredFeature) {
					json_object_push(language, "requiredFeature",
					                 json_string_new(table->languages[j]->requiredFeature->name));
				}
				json_value *features = json_array_new(table->languages[j]->featureCount);
				for (tableid_t k = 0; k < table->languages[j]->featureCount; k++)
					if (table->languages[j]->features[k]) {
						json_array_push(features, json_string_new(table->languages[j]->features[k]->name));
					}
				json_object_push(language, "features", preserialize(features));
				json_object_push(languages, table->languages[j]->name, language);
			}
			json_object_push(otl, "languages", languages);
		}
		loggedStep("Features") {
			// dump feature list
			json_value *features = json_object_new(table->featureCount);
			for (tableid_t j = 0; j < table->featureCount; j++) {
				json_value *feature = json_array_new(table->features[j]->lookupCount);
				for (tableid_t k = 0; k < table->features[j]->lookupCount; k++)
					if (table->features[j]->lookups[k]) {
						json_array_push(feature, json_string_new(table->features[j]->lookups[k]->name));
					}
				json_object_push(features, table->features[j]->name, preserialize(feature));
			}
			json_object_push(otl, "features", features);
		}
		loggedStep("Lookups") {
			// dump lookups
			json_value *lookups = json_object_new(table->lookupCount);
			json_value *lookupOrder = json_array_new(table->lookupCount);
			for (tableid_t j = 0; j < table->lookupCount; j++) {
				json_value *lookup = json_object_new(5);
				_dump_lookup(table->lookups[j], lookup);
				json_object_push(lookups, table->lookups[j]->name, lookup);
				json_array_push(lookupOrder, json_string_new(table->lookups[j]->name));
			}
			json_object_push(otl, "lookups", lookups);
			json_object_push(otl, "lookupOrder", lookupOrder);
		}
		json_object_push(root, tag, otl);
	}
}
