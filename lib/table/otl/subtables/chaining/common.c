#include "../chaining.h"
#include "common.h"
void otl_init_chaining(subtable_chaining *subtable) {
	memset(subtable, 0, sizeof(*subtable));
}
void otl_dispose_chaining(subtable_chaining *subtable) {
	if (subtable->type) {
		if (subtable->rules) {
			for (tableid_t j = 0; j < subtable->rulesCount; j++) {
				deleteRule(subtable->rules[j]);
			}
			FREE(subtable->rules);
		}
		if (subtable->bc) { ClassDef.destroy(subtable->bc); }
		if (subtable->ic) { ClassDef.destroy(subtable->ic); }
		if (subtable->fc) { ClassDef.destroy(subtable->fc); }
	} else {
		closeRule(&subtable->rule);
	}
}

caryll_standardRefType(subtable_chaining, otl_init_chaining, otl_dispose_chaining, iSubtable_chaining);
