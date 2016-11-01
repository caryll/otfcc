#include "private.h"

#define DELETE_TYPE(type, fn)                                                                                          \
	case type:                                                                                                         \
		fn(lookup->subtables[j]);                                                                                      \
		break;
void otfcc_delete_lookup(otl_Lookup *lookup) {
	if (!lookup) return;
	if (lookup->subtables) {
		for (tableid_t j = 0; j < lookup->subtableCount; j++) {
			switch (lookup->type) {
				DELETE_TYPE(otl_type_gsub_single, otl_delete_gsub_single);
				DELETE_TYPE(otl_type_gsub_multiple, otl_delete_gsub_multi);
				DELETE_TYPE(otl_type_gsub_alternate, otl_delete_gsub_multi);
				DELETE_TYPE(otl_type_gsub_ligature, otl_delete_gsub_ligature);
				DELETE_TYPE(otl_type_gsub_chaining, otl_delete_chaining);
				DELETE_TYPE(otl_type_gsub_reverse, otl_delete_gsub_reverse);
				DELETE_TYPE(otl_type_gpos_single, otl_delete_gpos_single);
				DELETE_TYPE(otl_type_gpos_pair, otl_delete_gpos_pair);
				DELETE_TYPE(otl_type_gpos_cursive, otl_delete_gpos_cursive);
				DELETE_TYPE(otl_type_gpos_chaining, otl_delete_chaining);
				DELETE_TYPE(otl_type_gpos_markToBase, otl_delete_gpos_markToSingle);
				DELETE_TYPE(otl_type_gpos_markToMark, otl_delete_gpos_markToSingle);
				DELETE_TYPE(otl_type_gpos_markToLigature, otl_delete_gpos_markToLigature);
				default:;
			}
		}
		FREE(lookup->subtables);
		sdsfree(lookup->name);
	}
	FREE(lookup);
}

// COMMON PART
table_OTL *otfcc_newOtl() {
	table_OTL *table;
	NEW(table);
	table->languageCount = 0;
	table->languages = NULL;
	table->featureCount = 0;
	table->features = NULL;
	table->lookupCount = 0;
	table->lookups = NULL;
	return table;
}

void otfcc_deleteOtl(table_OTL *table) {
	if (!table) return;
	if (table->languages) {
		for (tableid_t j = 0; j < table->languageCount; j++) {
			if (table->languages[j]->name) sdsfree(table->languages[j]->name);
			if (table->languages[j]->features) FREE(table->languages[j]->features);
			FREE(table->languages[j]);
		}
		FREE(table->languages);
	}
	if (table->features) {
		for (tableid_t j = 0; j < table->featureCount; j++) {
			if (table->features[j]->name) sdsfree(table->features[j]->name);
			if (table->features[j]->lookups) FREE(table->features[j]->lookups);
			FREE(table->features[j]);
		}
		FREE(table->features);
	}
	if (table->lookups) {
		for (tableid_t j = 0; j < table->lookupCount; j++) {
			otfcc_delete_lookup(table->lookups[j]);
		}
		FREE(table->lookups);
	}
	FREE(table);
}
