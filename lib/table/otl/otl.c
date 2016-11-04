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
static void deleteLookupListItem(otl_Lookup **entry) {
	otfcc_delete_lookup(*entry);
}
static void deleteFeature(otl_Feature **feature) {
	if (!*feature) return;
	if ((*feature)->name) sdsfree((*feature)->name);
	if ((*feature)->lookups) FREE((*feature)->lookups);
	FREE(*feature);
}
static void deleteLanguage(otl_LanguageSystem **language) {
	if (!*language) return;
	if ((*language)->name) sdsfree((*language)->name);
	if ((*language)->features) FREE((*language)->features);
	FREE(*language);
}

otl_LookupTI_t otl_LookupTI = {
    .ctor = NULL, .copyctor = NULL, .dtor = deleteLookupListItem,
};
otl_FeatureTI_t otl_FeatureTI = {
    .ctor = NULL, .copyctor = NULL, .dtor = deleteFeature,
};
otl_LanguageSystemTI_t otl_LanguageSystemTI = {
    .ctor = NULL, .copyctor = NULL, .dtor = deleteLanguage,
};

// COMMON PART
table_OTL *otfcc_newOtl() {
	table_OTL *table;
	NEW(table);
	caryll_vecInit(&table->lookups, otl_LookupTI);
	caryll_vecInit(&table->features, otl_FeatureTI);
	caryll_vecInit(&table->languages, otl_LanguageSystemTI);
	return table;
}

void otfcc_deleteOtl(table_OTL *table) {
	if (!table) return;
	caryll_vecReset(&table->lookups);
	caryll_vecReset(&table->features);
	caryll_vecReset(&table->languages);
	FREE(table);
}
