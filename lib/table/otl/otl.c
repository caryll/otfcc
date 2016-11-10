#include "private.h"

#define DELETE_TYPE(type, fn)                                                                                          \
	case type:                                                                                                         \
		((void (*)(otl_Subtable *))fn)(*subtableRef);                                                                  \
		break;

static void disposeSubtableDependent(MODIFY otl_SubtablePtr *subtableRef, const otl_Lookup *lookup) {
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
static caryll_ElementInterface(otl_SubtablePtr) otl_iSubtablePtr = {
    .init = NULL, .copy = NULL, .dispose = NULL,
};
caryll_VectorImplDestroyDependent(otl_SubtableList, otl_SubtablePtr, otl_Lookup, disposeSubtableDependent);
caryll_VectorImplFunctions(otl_SubtableList, otl_SubtablePtr, otl_iSubtablePtr);
caryll_VectorInterfaceTypeName(otl_SubtableList) otl_iSubtableList = {
    caryll_VectorImplAssignments(otl_SubtableList, otl_SubtablePtr, otl_iSubtablePtr),
    .disposeDependent = otl_SubtableList_disposeDependent,
};

void otfcc_delete_lookup(otl_Lookup *lookup) {
	if (!lookup) return;
	otl_iSubtableList.disposeDependent(&lookup->subtables, lookup);
	sdsfree(lookup->name);
	FREE(lookup);
}

// LOOKUP
static void initLookupPtr(otl_LookupPtr *entry) {
	NEW(*entry);
	(*entry)->name = NULL;
	otl_iSubtableList.init(&(*entry)->subtables);
}
static void disposeLookupPtr(otl_LookupPtr *entry) {
	otfcc_delete_lookup(*entry);
}
caryll_ElementInterfaceOf(otl_LookupPtr) otl_iLookup = {
    .init = initLookupPtr, .dispose = disposeLookupPtr,
};
caryll_TrivialElementImpl(otl_LookupRef, otl_iLookupRef);
caryll_DefineVectorImpl(otl_LookupRefList, otl_LookupRef, otl_iLookupRef, otl_iLookupRefList);

// FEATURE
static void initFeaturePtr(otl_FeaturePtr *feature) {
	NEW(*feature);
	otl_iLookupRefList.init(&(*feature)->lookups);
}
static void disposeFeaturePtr(otl_Feature **feature) {
	if (!*feature) return;
	if ((*feature)->name) sdsfree((*feature)->name);
	otl_iLookupRefList.dispose(&(*feature)->lookups);
	FREE(*feature);
}
caryll_ElementInterfaceOf(otl_FeaturePtr) otl_iFeature = {
    .init = initFeaturePtr, .dispose = disposeFeaturePtr,
};
caryll_TrivialElementImpl(otl_FeatureRef, otl_iFeatureRef);
caryll_DefineVectorImpl(otl_FeatureRefList, otl_FeatureRef, otl_iFeatureRef, otl_iFeatureRefList);

// LANGUAGE
static void initLanguagePtr(otl_LanguageSystemPtr *language) {
	NEW(*language);
	otl_iFeatureRefList.init(&(*language)->features);
}
static void disposeLanguagePtr(otl_LanguageSystemPtr *language) {
	if (!*language) return;
	if ((*language)->name) sdsfree((*language)->name);
	otl_iFeatureRefList.dispose(&(*language)->features);
	FREE(*language);
}
caryll_ElementInterfaceOf(otl_LanguageSystemPtr) otl_iLanguageSystem = {
    .init = initLanguagePtr, .dispose = disposeLanguagePtr,
};

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
