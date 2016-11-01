#include "../chaining.h"
#include "common.h"

void otl_delete_chaining(otl_Subtable *_subtable) {
	if (_subtable) {
		subtable_chaining *subtable = &(_subtable->chaining);
		if (subtable->type) {
			if (subtable->rules) {
				for (tableid_t j = 0; j < subtable->rulesCount; j++) {
					deleteRule(subtable->rules[j]);
				}
				FREE(subtable->rules);
			}
			if (subtable->bc) { ClassDef.dispose(subtable->bc); }
			if (subtable->ic) { ClassDef.dispose(subtable->ic); }
			if (subtable->fc) { ClassDef.dispose(subtable->fc); }
			FREE(_subtable);
		} else {
			closeRule(&subtable->rule);
		}
	}
}
