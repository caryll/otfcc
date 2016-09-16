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
		if (rule->apply[j].lookup.name) {
		FIND_LOOKUP:;
			for (tableid_t k = 0; k < table->lookupCount; k++) {
				if (strcmp(table->lookups[k]->name, rule->apply[j].lookup.name) != 0) continue;
				foundLookup = true;
				handle_consolidateTo(&rule->apply[j].lookup, k, table->lookups[k]->name);
			}
			if (!foundLookup) {
				// Maybe the lookup is aliased.
				for (tableid_t k = 0; k < table->lookupAliasesCount; k++) {
					if (strcmp(table->lookupAliases[k].from, rule->apply[j].lookup.name) != 0) continue;
					handle_delete(&rule->apply[j].lookup);
					rule->apply[j].lookup = handle_fromName(sdsdup(table->lookupAliases[k].to));
					goto FIND_LOOKUP;
				}
			}
		}
		if (!foundLookup && rule->apply[j].lookup.name) {
			fprintf(stderr, "[Consolidate] Quoting an invalid lookup %s in lookup %s.\n", rule->apply[j].lookup.name,
			        lookupName);
			handle_delete(&rule->apply[j].lookup);
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
typedef struct {
	int gid;
	sds gname;
	int cls;
	UT_hash_handle hh;
} classifier_hash;
static int by_gid_clsh(classifier_hash *a, classifier_hash *b) {
	return a->gid - b->gid;
}

static int classCompatible(classifier_hash **h, otl_Coverage *cov, int *past) {
	// checks whether a coverage is compatible to a class hash.
	classifier_hash *s;
	if (cov->numGlyphs == 0) return 1;
	int gid = cov->glyphs[0].index;
	// check pass
	HASH_FIND_INT(*h, &gid, s);
	if (s) {
		// the coverage has been defined into a class
		classifier_hash *ss, *tmp;
		for (glyphid_t j = 1; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].index;
			HASH_FIND_INT(*h, &gid, ss);
			if (!ss || ss->cls != s->cls) return 0;
		}
		// reverse check: all glyphs classified are there in the coverage
		classifier_hash *revh = NULL;
		for (glyphid_t j = 0; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].index;
			classifier_hash *rss;
			HASH_FIND_INT(revh, &gid, rss);
			if (!rss) {
				NEW(rss);
				rss->gid = gid;
				rss->gname = cov->glyphs[j].name;
				rss->cls = s->cls;
				HASH_ADD_INT(revh, gid, rss);
			}
		}

		bool allcheck = true;
		foreach_hash(ss, *h) if (ss->cls == s->cls) {
			int gid = ss->gid;
			classifier_hash *rss;
			HASH_FIND_INT(revh, &gid, rss);
			if (!rss) {
				allcheck = false;
				break;
			}
		}
		HASH_ITER(hh, revh, ss, tmp) {
			HASH_DEL(revh, ss);
			free(ss);
		}
		return allcheck ? s->cls : 0;
	} else {
		// the coverage is not defined into a class.
		classifier_hash *ss;
		for (glyphid_t j = 1; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].index;
			HASH_FIND_INT(*h, &gid, ss);
			if (ss) return 0;
		}
		for (glyphid_t j = 0; j < cov->numGlyphs; j++) {
			classifier_hash *s;
			NEW(s);
			s->gid = cov->glyphs[j].index;
			s->gname = cov->glyphs[j].name;
			s->cls = *past + 1;
			HASH_ADD_INT(*h, gid, s);
		}
		*past += 1;
		return 1;
	}
}
static void rewriteRule(otl_ChainingRule *rule, classifier_hash *hb, classifier_hash *hi, classifier_hash *hf) {
	for (tableid_t m = 0; m < rule->matchCount; m++) {
		if (rule->match[m]->numGlyphs > 0) {
			classifier_hash *h = (m < rule->inputBegins ? hb : m < rule->inputEnds ? hi : hf);
			classifier_hash *s;
			int gid = rule->match[m]->glyphs[0].index;
			HASH_FIND_INT(h, &gid, s);
			otl_delete_Coverage(rule->match[m]);
			NEW(rule->match[m]);
			rule->match[m]->numGlyphs = 1;
			NEW(rule->match[m]->glyphs);
			rule->match[m]->glyphs[0] = handle_fromIndex(s->cls);
		} else {
			otl_delete_Coverage(rule->match[m]);
			NEW(rule->match[m]);
			rule->match[m]->numGlyphs = 1;
			NEW(rule->match[m]->glyphs);
			rule->match[m]->glyphs[0] = handle_fromIndex(0);
		}
	}
}
static otl_ClassDef *toClass(classifier_hash *h) {
	otl_ClassDef *cd;
	NEW(cd);
	cd->numGlyphs = HASH_COUNT(h);
	if (!cd->numGlyphs) {
		cd->glyphs = NULL;
		cd->classes = NULL;
		return cd;
	}
	NEW_N(cd->glyphs, cd->numGlyphs);
	NEW_N(cd->classes, cd->numGlyphs);
	classifier_hash *item;
	glyphclass_t maxclass = 0;
	glyphid_t j = 0;
	HASH_SORT(h, by_gid_clsh);
	foreach_hash(item, h) {
		cd->glyphs[j] = handle_fromConsolidated(item->gid, item->gname);
		cd->classes[j] = item->cls;
		if (item->cls > maxclass) maxclass = item->cls;
		j++;
	}
	cd->maxclass = maxclass;
	return cd;
}
void classify_around(otl_Lookup *lookup, tableid_t j) {
	classifier_hash *hb = NULL;
	classifier_hash *hi = NULL;
	classifier_hash *hf = NULL;
	if (!lookup->subtables[j] || lookup->subtables[j]->chaining.classified) return;
	// initialize the class hash
	subtable_chaining *subtable0 = &(lookup->subtables[j]->chaining);
	int classno_b = 0;
	int classno_i = 0;
	int classno_f = 0;
	bool *compatibility = NULL;
	otl_ChainingRule *rule0 = subtable0->rules[0];
	for (tableid_t m = 0; m < rule0->matchCount; m++) {
		int check = 0;
		if (m < rule0->inputBegins) {
			check = classCompatible(&hb, rule0->match[m], &classno_b);
		} else if (m < rule0->inputEnds) {
			check = classCompatible(&hi, rule0->match[m], &classno_i);
		} else {
			check = classCompatible(&hf, rule0->match[m], &classno_f);
		}
		if (!check) { goto FAIL; }
	}
	compatibility = calloc(lookup->subtableCount, sizeof(bool));
	tableid_t compatibleCount = 0;
	for (tableid_t k = j + 1; k < lookup->subtableCount; k++) {
		if (lookup->subtables[k] && !lookup->subtables[k]->chaining.classified) {
			otl_ChainingRule *rule = lookup->subtables[k]->chaining.rules[0];
			bool allcheck = true;
			for (tableid_t m = 0; m < rule->matchCount; m++) {
				int check = 0;
				if (m < rule->inputBegins) {
					check = classCompatible(&hb, rule->match[m], &classno_b);
				} else if (m < rule->inputEnds) {
					check = classCompatible(&hi, rule->match[m], &classno_i);
				} else {
					check = classCompatible(&hf, rule->match[m], &classno_f);
				}
				if (!check) {
					allcheck = false;
					goto endcheck;
				}
			}
			if (allcheck) { compatibility[k] = true, compatibleCount += 1; }
		}
	}
endcheck:
	if (compatibleCount > 1) {
		compatibility[j] = true;
		free(subtable0->rules);
		NEW_N(subtable0->rules, compatibleCount + 1);
		subtable0->rulesCount = compatibleCount + 1;

		subtable0->rules[0] = rule0;
		rewriteRule(rule0, hb, hi, hf);
		// write other rules
		tableid_t kk = 1;
		for (tableid_t k = j + 1; k < lookup->subtableCount; k++)
			if (compatibility[k]) {
				otl_ChainingRule *rule = lookup->subtables[k]->chaining.rules[0];
				subtable0->rules[kk] = rule;
				rewriteRule(rule, hb, hi, hf);
				lookup->subtables[k]->chaining.rules[0] = NULL;
				otl_delete_chaining(lookup->subtables[k]);
				lookup->subtables[k] = NULL;
				kk++;
			}

		subtable0->classified = true;
		subtable0->bc = toClass(hb);
		subtable0->ic = toClass(hi);
		subtable0->fc = toClass(hf);
	}
FAIL:;
	if (compatibility) free(compatibility);
	if (hb) {
		classifier_hash *s, *tmp;
		HASH_ITER(hh, hb, s, tmp) {
			HASH_DEL(hb, s);
			free(s);
		}
	}
	if (hi) {
		classifier_hash *s, *tmp;
		HASH_ITER(hh, hi, s, tmp) {
			HASH_DEL(hi, s);
			free(s);
		}
	}
	if (hf) {
		classifier_hash *s, *tmp;
		HASH_ITER(hh, hf, s, tmp) {
			HASH_DEL(hf, s);
			free(s);
		}
	}
	return;
}
void fontop_classifyChainings(otl_Lookup *lookup) {
	// in this procedure we will replace the subtables' content to classes.
	// This can massively reduce the size of the lookup.
	// Remember, this process is completely automatic.
	#ifndef DEBUG
	for (tableid_t j = 0; j < lookup->subtableCount; j++) {
		classify_around(lookup, j);
	}
	#endif
}
