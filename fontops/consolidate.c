#include "consolidate.h"
// Consolidation
// Replace name entries in json to gid and do some check
void caryll_font_consolidate_glyph(glyf_glyph *g, caryll_font *font) {
	uint16_t nReferencesConsolidated = 0;
	for (uint16_t j = 0; j < g->numberOfReferences; j++) {
		glyph_order_entry *entry = NULL;
		glyf_reference *r = &(g->references[j]);
		if (r->glyph.name) {
			HASH_FIND_STR(*font->glyph_order, r->glyph.name, entry);
			if (entry) {
				r->glyph.gid = entry->gid;
				sdsfree(r->glyph.name);
				r->glyph.name = entry->name;
				nReferencesConsolidated += 1;
			} else {
				fprintf(stderr, "[Consolidate] Ignored absent glyph component reference /%s within /%s.\n",
				        r->glyph.name, g->name);
				r->glyph.gid = 0;
				sdsfree(r->glyph.name);
				r->glyph.name = NULL;
			}
		} else {
			r->glyph.gid = 0;
			r->glyph.name = NULL;
		}
	}
	if (nReferencesConsolidated < g->numberOfReferences) {
		if (nReferencesConsolidated == 0) {
			free(g->references);
			g->references = NULL;
			g->numberOfReferences = 0;
		} else {
			glyf_reference *consolidatedReferences = calloc(nReferencesConsolidated, sizeof(glyf_reference));
			for (uint16_t j = 0, k = 0; j < g->numberOfReferences; j++) {
				if (g->references[j].glyph.name) { consolidatedReferences[k++] = g->references[j]; }
			}
			free(g->references);
			g->references = consolidatedReferences;
			g->numberOfReferences = nReferencesConsolidated;
		}
	}
}
void caryll_font_consolidate_glyf(caryll_font *font) {
	if (font->glyph_order && *font->glyph_order && font->glyf) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			if (font->glyf->glyphs[j]) {
				caryll_font_consolidate_glyph(font->glyf->glyphs[j], font);
			} else {
				font->glyf->glyphs[j] = caryll_new_glyf_glhph();
			}
		}
	}
}

void caryll_font_consolidate_cmap(caryll_font *font) {
	if (font->glyph_order && *font->glyph_order && font->cmap) {
		cmap_entry *item;
		foreach_hash(item, *font->cmap) if (item->glyph.name) {
			glyph_order_entry *ordentry;
			HASH_FIND_STR(*font->glyph_order, item->glyph.name, ordentry);
			if (ordentry) {
				item->glyph.gid = ordentry->gid;
				sdsfree(item->glyph.name);
				item->glyph.name = ordentry->name;
			} else {
				fprintf(stderr, "[Consolidate] Ignored mapping U+%04X to non-existent glyph /%s.\n", item->unicode,
				        item->glyph.name);
				item->glyph.gid = 0;
				sdsfree(item->glyph.name);
				item->glyph.name = NULL;
			}
		}
		else {
			item->glyph.gid = 0;
			item->glyph.name = NULL;
		}
	}
}

static void consolidate_coverage(caryll_font *font, otl_coverage *coverage, sds lookupName) {
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		glyph_order_entry *ordentry;
		HASH_FIND_STR(*font->glyph_order, coverage->glyphs[j].name, ordentry);
		if (ordentry) {
			coverage->glyphs[j].gid = ordentry->gid;
			sdsfree(coverage->glyphs[j].name);
			coverage->glyphs[j].name = ordentry->name;
		} else {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n", coverage->glyphs[j].name,
			        lookupName);
			coverage->glyphs[j].gid = 0;
			DELETE(sdsfree, coverage->glyphs[j].name);
		}
	}
}
static void shrink_coverage(otl_coverage *coverage) {
	uint16_t k = 0;
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		if (coverage->glyphs[j].name) coverage->glyphs[k++] = coverage->glyphs[j];
	}
	coverage->numGlyphs = k;
}

typedef struct {
	int fromid;
	sds fromname;
	int toid;
	sds toname;
	UT_hash_handle hh;
} gsub_single_map_hash;
static INLINE int by_from_id(gsub_single_map_hash *a, gsub_single_map_hash *b) { return a->fromid - b->fromid; }
static bool consolidate_gsub_single(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_gsub_single *subtable = &(_subtable->gsub_single);
	consolidate_coverage(font, subtable->from, lookupName);
	consolidate_coverage(font, subtable->to, lookupName);
	uint16_t len =
	    (subtable->from->numGlyphs < subtable->to->numGlyphs ? subtable->from->numGlyphs : subtable->from->numGlyphs);
	gsub_single_map_hash *h = NULL;
	for (uint16_t k = 0; k < len; k++) {
		if (subtable->from->glyphs[k].name && subtable->to->glyphs[k].name) {
			gsub_single_map_hash *s;
			int fromid = subtable->from->glyphs[k].gid;
			HASH_FIND_INT(h, &fromid, s);
			if (s) {
				fprintf(stderr, "[Consolidate] Double-mapping a glyph in a single substitution /%s.\n",
				        subtable->from->glyphs[k].name);
			} else {
				NEW(s);
				s->fromid = subtable->from->glyphs[k].gid;
				s->toid = subtable->to->glyphs[k].gid;
				s->fromname = subtable->from->glyphs[k].name;
				s->toname = subtable->to->glyphs[k].name;
				HASH_ADD_INT(h, fromid, s);
			}
		}
	}
	HASH_SORT(h, by_from_id);
	if (HASH_COUNT(h) != subtable->from->numGlyphs || HASH_COUNT(h) != subtable->to->numGlyphs) {
		fprintf(stderr, "[Consolidate] In single subsitution lookup %s, some mappings are ignored.\n", lookupName);
	}
	subtable->from->numGlyphs = HASH_COUNT(h);
	subtable->to->numGlyphs = HASH_COUNT(h);
	FREE(subtable->from->glyphs);
	FREE(subtable->to->glyphs);
	NEW_N(subtable->from->glyphs, subtable->from->numGlyphs);
	NEW_N(subtable->to->glyphs, subtable->to->numGlyphs);
	{
		gsub_single_map_hash *s, *tmp;
		uint16_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			subtable->from->glyphs[j].gid = s->fromid;
			subtable->from->glyphs[j].name = s->fromname;
			subtable->to->glyphs[j].gid = s->toid;
			subtable->to->glyphs[j].name = s->toname;
			j++;
			HASH_DEL(h, s);
			free(s);
		}
	}
	return false;
}

typedef struct {
	int fromid;
	sds fromname;
	otl_coverage *to;
	UT_hash_handle hh;
} gsub_multi_hash;
static INLINE int by_from_id_multi(gsub_multi_hash *a, gsub_multi_hash *b) { return a->fromid - b->fromid; }
static bool consolidate_gsub_multi(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	consolidate_coverage(font, subtable->from, lookupName);
	for (uint16_t j = 0; j < subtable->from->numGlyphs; j++) {
		consolidate_coverage(font, subtable->to[j], lookupName);
		shrink_coverage(subtable->to[j]);
	}
	gsub_multi_hash *h = NULL;
	for (uint16_t k = 0; k < subtable->from->numGlyphs; k++) {
		if (subtable->from->glyphs[k].name) {
			gsub_multi_hash *s;
			int fromid = subtable->from->glyphs[k].gid;
			HASH_FIND_INT(h, &fromid, s);
			if (!s) {
				NEW(s);
				s->fromid = subtable->from->glyphs[k].gid;
				s->fromname = subtable->from->glyphs[k].name;
				s->to = subtable->to[k];
				HASH_ADD_INT(h, fromid, s);
			} else {
				caryll_delete_coverage(subtable->to[k]);
			}
		} else {
			caryll_delete_coverage(subtable->to[k]);
		}
	}
	HASH_SORT(h, by_from_id_multi);
	subtable->from->numGlyphs = HASH_COUNT(h);
	{
		gsub_multi_hash *s, *tmp;
		uint16_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			subtable->from->glyphs[j].gid = s->fromid;
			subtable->from->glyphs[j].name = s->fromname;
			subtable->to[j] = s->to;
			j++;
			HASH_DEL(h, s);
			free(s);
		}
	}
	return false;
}

static bool consolidate_gsub_ligature(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_gsub_ligature *subtable = &(_subtable->gsub_ligature);
	consolidate_coverage(font, subtable->to, lookupName);
	for (uint16_t j = 0; j < subtable->to->numGlyphs; j++) {
		consolidate_coverage(font, subtable->from[j], lookupName);
		shrink_coverage(subtable->from[j]);
	}
	uint16_t jj = 0;
	for (uint16_t k = 0; k < subtable->to->numGlyphs; k++) {
		if (subtable->to->glyphs[k].name && subtable->from[k]->numGlyphs) {
			subtable->to->glyphs[jj] = subtable->to->glyphs[k];
			subtable->from[jj] = subtable->from[k];
			jj++;
		} else {
			caryll_delete_coverage(subtable->from[k]);
		}
	}
	subtable->to->numGlyphs = jj;
	return false;
}

typedef struct {
	int gid;
	sds name;
	otl_mark_record markrec;
	UT_hash_handle hh;
} mark_hash;
static INLINE int mark_by_gid(mark_hash *a, mark_hash *b) { return a->gid - b->gid; }
typedef struct {
	int gid;
	sds name;
	otl_anchor *anchors;
	UT_hash_handle hh;
} base_hash;
static INLINE int base_by_gid(base_hash *a, base_hash *b) { return a->gid - b->gid; }
static bool consolidate_mark_to_single(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_gpos_mark_to_single *subtable = &(_subtable->gpos_mark_to_single);
	consolidate_coverage(font, subtable->marks, lookupName);
	consolidate_coverage(font, subtable->bases, lookupName);
	{ // consolidate marks
		mark_hash *hm = NULL;
		for (uint16_t k = 0; k < subtable->marks->numGlyphs; k++) {
			if (subtable->marks->glyphs[k].name) {
				mark_hash *s = NULL;
				HASH_FIND_INT(hm, &(subtable->marks->glyphs[k].gid), s);
				if (!s && subtable->markArray->records[k].anchor.present &&
				    subtable->markArray->records[k].markClass < subtable->classCount) {
					NEW(s);
					s->gid = subtable->marks->glyphs[k].gid;
					s->name = subtable->marks->glyphs[k].name;
					s->markrec = subtable->markArray->records[k];
					HASH_ADD_INT(hm, gid, s);
				} else {
					fprintf(stderr,
					        "[Consolidate] Ignored invalid or double-mapping mark definition for /%s in lookup %s.\n",
					        subtable->marks->glyphs[k].name, lookupName);
				}
			}
		}
		HASH_SORT(hm, mark_by_gid);
		subtable->marks->numGlyphs = HASH_COUNT(hm);
		subtable->markArray->markCount = HASH_COUNT(hm);
		mark_hash *s, *tmp;
		uint16_t k = 0;
		HASH_ITER(hh, hm, s, tmp) {
			subtable->marks->glyphs[k].gid = s->gid;
			subtable->marks->glyphs[k].name = s->name;
			subtable->markArray->records[k] = s->markrec;
			k++;
			HASH_DEL(hm, s);
			free(s);
		}
	}
	{ // consolidate bases
		base_hash *hm = NULL;
		for (uint16_t k = 0; k < subtable->bases->numGlyphs; k++) {
			if (subtable->bases->glyphs[k].name) {
				base_hash *s = NULL;
				HASH_FIND_INT(hm, &(subtable->bases->glyphs[k].gid), s);
				if (!s) {
					NEW(s);
					s->gid = subtable->bases->glyphs[k].gid;
					s->name = subtable->bases->glyphs[k].name;
					s->anchors = subtable->baseArray[k];
					HASH_ADD_INT(hm, gid, s);
				} else {
					free(subtable->baseArray[k]);
					fprintf(stderr, "[Consolidate] Ignored anchor double-definition for /%s in lookup %s.\n",
					        subtable->bases->glyphs[k].name, lookupName);
				}
			} else {
				free(subtable->baseArray[k]);
			}
		}
		HASH_SORT(hm, base_by_gid);
		subtable->bases->numGlyphs = HASH_COUNT(hm);
		base_hash *s, *tmp;
		uint16_t k = 0;
		HASH_ITER(hh, hm, s, tmp) {
			subtable->bases->glyphs[k].gid = s->gid;
			subtable->bases->glyphs[k].name = s->name;
			subtable->baseArray[k] = s->anchors;
			k++;
			HASH_DEL(hm, s);
			free(s);
		}
	}
	return false;
}

static bool consolidate_gsub_chaining(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_chaining *subtable = &(_subtable->chaining);
	otl_chaining_rule *rule = subtable->rules[0];
	for (uint16_t j = 0; j < rule->matchCount; j++) {
		consolidate_coverage(font, rule->match[j], lookupName);
		shrink_coverage(rule->match[j]);
	}
	if (rule->inputBegins < 0) rule->inputBegins = 0;
	if (rule->inputBegins > rule->matchCount) rule->inputBegins = rule->matchCount;
	if (rule->inputEnds < 0) rule->inputEnds = 0;
	if (rule->inputEnds > rule->matchCount) rule->inputEnds = rule->matchCount;
	for (uint16_t j = 0; j < rule->applyCount; j++) {
		bool foundLookup = false;
		if (rule->apply[j].lookupName) {
			for (uint16_t k = 0; k < table->lookupCount; k++)
				if (strcmp(table->lookups[k]->name, rule->apply[j].lookupName) == 0) {
					foundLookup = true;
					rule->apply[j].lookupIndex = k;
					DELETE(sdsfree, rule->apply[j].lookupName);
					rule->apply[j].lookupName = table->lookups[k]->name;
				}
		}
		if (!foundLookup && rule->apply[j].lookupName) {
			// fprintf(stderr, "[Consolidate] Quoting an invalid lookup %s in lookup %s.\n", rule->apply[j].lookupName,
			//        lookupName);
			DELETE(sdsfree, rule->apply[j].lookupName);
		}
	}
	uint16_t k = 0;
	for (uint16_t j = 0; j < rule->applyCount; j++)
		if (rule->apply[j].lookupName) { rule->apply[k++] = rule->apply[j]; }
	rule->applyCount = k;
	if (!rule->applyCount) {
		delete_otl_chaining_subtable(_subtable);
		return true;
	}
	return false;
}
typedef struct {
	int gid;
	sds gname;
	int cls;
	UT_hash_handle hh;
} classifier_hash;
static int by_gid_clsh(classifier_hash *a, classifier_hash *b) { return a->gid - b->gid; }

static int classCompatible(classifier_hash **h, otl_coverage *cov, int *past) {
	// checks whether a coverage is compatible to a class hash.
	classifier_hash *s;
	if (cov->numGlyphs == 0) return 1;
	int gid = cov->glyphs[0].gid;
	// check pass
	HASH_FIND_INT(*h, &gid, s);
	if (s) {
		// the coverage has been defined into a class
		classifier_hash *ss, *tmp;
		for (uint16_t j = 1; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].gid;
			HASH_FIND_INT(*h, &gid, ss);
			if (!ss || ss->cls != s->cls) return 0;
		}
		// reverse check: all glyphs classified are there in the coverage
		classifier_hash *revh = NULL;
		for (uint16_t j = 0; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].gid;
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
		for (uint16_t j = 1; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].gid;
			HASH_FIND_INT(*h, &gid, ss);
			if (ss) return 0;
		}
		for (uint16_t j = 0; j < cov->numGlyphs; j++) {
			classifier_hash *s;
			NEW(s);
			s->gid = cov->glyphs[j].gid;
			s->gname = cov->glyphs[j].name;
			s->cls = *past + 1;
			HASH_ADD_INT(*h, gid, s);
		}
		*past += 1;
		return 1;
	}
}
static void rewriteRule(otl_chaining_rule *rule, classifier_hash *hb, classifier_hash *hi, classifier_hash *hf) {
	for (uint16_t m = 0; m < rule->matchCount; m++)
		if (rule->match[m]->numGlyphs > 0) {
			classifier_hash *h = (m < rule->inputBegins ? hb : m < rule->inputEnds ? hi : hf);
			classifier_hash *s;
			int gid = rule->match[m]->glyphs[0].gid;
			HASH_FIND_INT(h, &gid, s);
			caryll_delete_coverage(rule->match[m]);
			NEW(rule->match[m]);
			rule->match[m]->numGlyphs = 1;
			NEW(rule->match[m]->glyphs);
			rule->match[m]->glyphs[0].gid = s->cls;
			rule->match[m]->glyphs[0].name = NULL;
		} else {
			caryll_delete_coverage(rule->match[m]);
			NEW(rule->match[m]);
			rule->match[m]->numGlyphs = 1;
			NEW(rule->match[m]->glyphs);
			rule->match[m]->glyphs[0].gid = 0;
			rule->match[m]->glyphs[0].name = NULL;
		}
}
static otl_classdef *toClass(classifier_hash *h) {
	otl_classdef *cd;
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
	uint16_t maxclass = 0;
	uint16_t j = 0;
	HASH_SORT(h, by_gid_clsh);
	foreach_hash(item, h) {
		cd->glyphs[j].gid = item->gid;
		cd->glyphs[j].name = item->gname;
		cd->classes[j] = item->cls;
		if (item->cls > maxclass) maxclass = item->cls;
		j++;
	}
	cd->maxclass = maxclass;
	return cd;
}
void classify_around(otl_lookup *lookup, uint16_t j) {
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
	otl_chaining_rule *rule0 = subtable0->rules[0];
	for (uint16_t m = 0; m < rule0->matchCount; m++) {
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
	uint16_t compatibleCount = 0;
	for (uint16_t k = j + 1; k < lookup->subtableCount; k++) {
		if (lookup->subtables[k] && !lookup->subtables[k]->chaining.classified) {
			otl_chaining_rule *rule = lookup->subtables[k]->chaining.rules[0];
			bool allcheck = true;
			for (uint16_t m = 0; m < rule->matchCount; m++) {
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
		fprintf(stderr, "[Autoclassifier] %d subtables in %s are class-compatible to subtable %d.", compatibleCount,
		        lookup->name, j);
		fprintf(stderr, " Class count : B%d I%d F%d.\n", classno_b, classno_i, classno_f);
		compatibility[j] = true;
		free(subtable0->rules);
		NEW_N(subtable0->rules, compatibleCount + 1);
		subtable0->rulesCount = compatibleCount + 1;

		subtable0->rules[0] = rule0;
		rewriteRule(rule0, hb, hi, hf);

		// write other rules
		uint16_t kk = 1;
		for (uint16_t k = j + 1; k < lookup->subtableCount; k++)
			if (compatibility[k]) {
				otl_chaining_rule *rule = lookup->subtables[k]->chaining.rules[0];
				subtable0->rules[kk] = rule;
				rewriteRule(rule, hb, hi, hf);
				lookup->subtables[k]->chaining.rules[0] = NULL;
				delete_otl_chaining_subtable(lookup->subtables[k]);
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
	classifier_hash *s, *tmp;
	HASH_ITER(hh, hb, s, tmp) {
		HASH_DEL(hb, s);
		free(s);
	}
	HASH_ITER(hh, hi, s, tmp) {
		HASH_DEL(hi, s);
		free(s);
	}
	HASH_ITER(hh, hf, s, tmp) {
		HASH_DEL(hf, s);
		free(s);
	}
	return;
}
void classify(otl_lookup *lookup) {
	// in this procedure we will replace the subtables' content to classes.
	// This can massively reduce the size of the lookup.
	// Remember, this process is completely automatic.
	for (uint16_t j = 0; j < lookup->subtableCount; j++) {
		classify_around(lookup, j);
	}
}

void declare_consolidate_type(otl_lookup_type type, bool (*fn)(caryll_font *, table_otl *, otl_subtable *, sds),
                              caryll_font *font, table_otl *table, otl_lookup *lookup) {
	if (lookup && lookup->subtableCount && lookup->type == type) {
		for (uint16_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) {
				bool subtableRemoved = fn(font, table, lookup->subtables[j], lookup->name);
				if (subtableRemoved) { lookup->subtables[j] = NULL; }
			}
		}
		uint16_t k = 0;
		for (uint16_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) { lookup->subtables[k++] = lookup->subtables[j]; }
		}
		lookup->subtableCount = k;

		if (lookup->subtableCount &&
		    (lookup->type == otl_type_gsub_chaining || lookup->type == otl_type_gpos_chaining)) {
			classify(lookup);
			uint16_t k = 0;
			for (uint16_t j = 0; j < lookup->subtableCount; j++) {
				if (lookup->subtables[j]) { lookup->subtables[k++] = lookup->subtables[j]; }
			}
			lookup->subtableCount = k;
		}
	}
}
void caryll_consolidate_lookup(caryll_font *font, table_otl *table, otl_lookup *lookup) {
	declare_consolidate_type(otl_type_gsub_single, consolidate_gsub_single, font, table, lookup);
	declare_consolidate_type(otl_type_gsub_multiple, consolidate_gsub_multi, font, table, lookup);
	declare_consolidate_type(otl_type_gsub_alternate, consolidate_gsub_multi, font, table, lookup);
	declare_consolidate_type(otl_type_gsub_ligature, consolidate_gsub_ligature, font, table, lookup);
	declare_consolidate_type(otl_type_gsub_chaining, consolidate_gsub_chaining, font, table, lookup);
	declare_consolidate_type(otl_type_gpos_chaining, consolidate_gsub_chaining, font, table, lookup);
	declare_consolidate_type(otl_type_gpos_mark_to_base, consolidate_mark_to_single, font, table, lookup);
	declare_consolidate_type(otl_type_gpos_mark_to_mark, consolidate_mark_to_single, font, table, lookup);
}

void caryll_font_consolidate_otl(caryll_font *font) {
	if (font->glyph_order && font->GSUB)
		for (uint16_t j = 0; j < font->GSUB->lookupCount; j++) {
			caryll_consolidate_lookup(font, font->GSUB, font->GSUB->lookups[j]);
		}
	if (font->glyph_order && font->GPOS)
		for (uint16_t j = 0; j < font->GPOS->lookupCount; j++) {
			caryll_consolidate_lookup(font, font->GPOS, font->GPOS->lookups[j]);
		}
}

void caryll_font_consolidate(caryll_font *font, caryll_dump_options *dumpopts) {
	caryll_font_consolidate_glyf(font);
	caryll_font_consolidate_cmap(font);
	caryll_font_consolidate_otl(font);
}
