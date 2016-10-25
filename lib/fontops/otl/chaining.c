#include "chaining.h"

bool consolidate_chaining(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName) {
	subtable_chaining *subtable = &(_subtable->chaining);
	otl_ChainingRule *rule = subtable->rules[0];
	for (tableid_t j = 0; j < rule->matchCount; j++) {
		fontop_consolidateCoverage(font, rule->match[j], lookupName);
		fontop_shrinkCoverage(rule->match[j], true);
	}
	if (rule->inputBegins > rule->matchCount) rule->inputBegins = rule->matchCount;
	if (rule->inputEnds > rule->matchCount) rule->inputEnds = rule->matchCount;
	for (tableid_t j = 0; j < rule->applyCount; j++) {
		bool foundLookup = false;
		lookup_handle *h = &(rule->apply[j].lookup);
		if (h->name) {
			for (tableid_t k = 0; k < table->lookupCount; k++) {
				if (strcmp(table->lookups[k]->name, h->name) != 0) continue;
				foundLookup = true;
				handle_consolidateTo(h, k, table->lookups[k]->name);
			}
			if (!foundLookup && rule->apply[j].lookup.name) {
				fprintf(stderr, "[Consolidate] Quoting an invalid lookup %s in lookup %s.\n",
				        rule->apply[j].lookup.name, lookupName);
				handle_delete(&rule->apply[j].lookup);
			}
		} else if (h->state == HANDLE_STATE_INDEX) {
			if (h->index >= table->lookupCount) h->index = 0;
			handle_consolidateTo(h, h->index, table->lookups[h->index]->name);
		}
	}
	// If a rule is designed to have no lookup application, it may be a ignoration
	// otfcc will keep them.
	if (rule->applyCount) {
		tableid_t k = 0;
		for (tableid_t j = 0; j < rule->applyCount; j++) {
			if (rule->apply[j].lookup.name) { rule->apply[k++] = rule->apply[j]; }
		}
		rule->applyCount = k;
		if (!rule->applyCount) { return true; }
	}
	return false;
}
