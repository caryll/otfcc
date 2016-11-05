#include "private.h"

#define DELETE_TYPE(type, fn)                                                                                          \
	case type:                                                                                                         \
		((void (*)(otl_Subtable *))fn)(lookup->subtables[j]);                                                          \
		break;

void otfcc_delete_lookup(otl_Lookup *lookup) {
	if (!lookup) return;
	if (lookup->subtables) {
		for (tableid_t j = 0; j < lookup->subtableCount; j++) {
			switch (lookup->type) {
				DELETE_TYPE(otl_type_gsub_single, iSubtable_gsub_single.destroy);
				DELETE_TYPE(otl_type_gsub_multiple, iSubtable_gsub_multi.destroy);
				DELETE_TYPE(otl_type_gsub_alternate, iSubtable_gsub_multi.destroy);
				DELETE_TYPE(otl_type_gsub_ligature, iSubtable_gsub_ligature.destroy);
				DELETE_TYPE(otl_type_gsub_chaining, iSubtable_chaining.destroy);
				DELETE_TYPE(otl_type_gsub_reverse, iSubtable_gsub_reverse.destroy);
				DELETE_TYPE(otl_type_gpos_single, iSubtable_gpos_single.destroy);
				DELETE_TYPE(otl_type_gpos_pair, iSubtable_gpos_pair.destroy);
				DELETE_TYPE(otl_type_gpos_cursive, iSubtable_gpos_cursive.destroy);
				DELETE_TYPE(otl_type_gpos_chaining, iSubtable_chaining.destroy);
				DELETE_TYPE(otl_type_gpos_markToBase, iSubtable_gpos_markToSingle.destroy);
				DELETE_TYPE(otl_type_gpos_markToMark, iSubtable_gpos_markToSingle.destroy);
				DELETE_TYPE(otl_type_gpos_markToLigature, iSubtable_gpos_markToLigature.destroy);
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

caryll_DtorElementImpl(otl_LookupPtr, deleteLookupListItem, otl_iLookup);
caryll_DtorElementImpl(otl_FeaturePtr, deleteFeature, otl_iFeature);
caryll_DtorElementImpl(otl_LanguageSystemPtr, deleteLanguage, otl_iLanguageSystem);

caryll_DefineVectorImpl(otl_LookupList, otl_LookupPtr, otl_iLookup, otl_iLookupList);
caryll_DefineVectorImpl(otl_FeatureList, otl_FeaturePtr, otl_iFeature, otl_iFeatureList);
caryll_DefineVectorImpl(otl_LangSystemList, otl_LanguageSystemPtr, otl_iLanguageSystem, otl_iLangSystemList);

// COMMON PART
table_OTL *otfcc_newOtl() {
	table_OTL *table;
	NEW(table);
	otl_iLookupList.init(&table->lookups);
	otl_iFeatureList.init(&table->features);
	otl_iLangSystemList.init(&table->languages);
	return table;
}

void otfcc_deleteOtl(table_OTL *table) {
	if (!table) return;
	otl_iLookupList.dispose(&table->lookups);
	otl_iFeatureList.dispose(&table->features);
	otl_iLangSystemList.dispose(&table->languages);
	FREE(table);
}
