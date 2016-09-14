#include "subr.h"

void joinNodes(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n);

void printNode(cff_SubrNode *n, bool nl) {
	if (n->guard) printf("GUARD ");
	if (n->hard) printf("HARD ");
	if (n->rule) {
		printf("[%d] ", n->rule->uniqueIndex);
	} else
		for (uint32_t j = 0; j < buflen(n->terminal); j++) {
			printf("%02x ", n->terminal->data[j]);
		}
	if (nl)
		printf("\n");
	else
		printf(". ");
}
void flatprintRule(cff_SubrRule *r) {
	for (cff_SubrNode *e = r->guard->next; e != r->guard; e = e->next) {
		if (e->rule) {
			flatprintRule(e->rule);
		} else {
			printNode(e, false);
		}
	}
}
void printRule(cff_SubrRule *r) {
	printf("[%d] -> ", r->uniqueIndex);
	for (cff_SubrNode *e = r->guard->next; e != r->guard; e = e->next) {
		printNode(e, false);
	}
	printf("\n");
	// flatprintRule(r);
	// printf("\n");
	r->printed = true;
	for (cff_SubrNode *e = r->guard->next; e != r->guard; e = e->next) {
		if (e->rule && !e->rule->printed) { printRule(e->rule); }
	}
}

static uint8_t *getSingletHashKey(cff_SubrNode *n, size_t *len) {
	size_t l1;
	if (n->rule) {
		l1 = sizeof(n->rule->uniqueIndex);
	} else {
		l1 = buflen(n->terminal) * sizeof(uint8_t);
	}

	*len = 3 + l1;
	uint8_t *key = malloc(*len);
	key[0] = '1';
	key[1] = (n->rule ? '1' : '0');
	key[2] = 0;
	if (n->rule) {
		memcpy(key + 3, &(n->rule->uniqueIndex), sizeof(n->rule->uniqueIndex));
	} else {
		memcpy(key + 3, n->terminal->data, buflen(n->terminal) * sizeof(uint8_t));
	}
	return key;
}

static uint8_t *getDoubletHashKey(cff_SubrNode *n, size_t *len) {
	size_t l1, l2;
	if (n->rule) {
		l1 = sizeof(n->rule->uniqueIndex);
	} else {
		l1 = buflen(n->terminal) * sizeof(uint8_t);
	}
	if (n->next->rule) {
		l2 = sizeof(n->next->rule->uniqueIndex);
	} else {
		l2 = buflen(n->next->terminal) * sizeof(uint8_t);
	}
	*len = 3 + l1 + l2 + 1;
	uint8_t *key = malloc(*len);
	key[0] = '2';
	key[1] = (n->rule ? '1' : '0');
	key[2] = (n->next->rule ? '1' : '0');
	key[*len - 1] = 0;
	if (n->rule) {
		memcpy(key + 3, &(n->rule->uniqueIndex), sizeof(n->rule->uniqueIndex));
	} else {
		memcpy(key + 3, n->terminal->data, buflen(n->terminal) * sizeof(uint8_t));
	}
	if (n->next->rule) {
		memcpy(key + 3 + l1, &(n->next->rule->uniqueIndex), sizeof(n->next->rule->uniqueIndex));
	} else {
		memcpy(key + 3 + l1, n->next->terminal->data, buflen(n->next->terminal) * sizeof(uint8_t));
	}
	return key;
}

void insertNodeAfter(cff_SubrNode *a, cff_SubrNode *b) {
	cff_SubrNode *n = a->next;
	b->next = n;
	n->prev = b;
	a->next = b;
	b->prev = a;
}

void clean_Node(cff_SubrNode *x) {
	if (x->rule) { x->rule->refcount -= 1; }
	x->rule = NULL;
	buffree(x->terminal);
	x->terminal = NULL;
}
void delete_Node(cff_SubrNode *x) {
	if (!x) return;
	clean_Node(x);
	FREE(x);
}

cff_SubrNode *cff_new_Node() {
	cff_SubrNode *n;
	NEW_CLEAN(n);
	n->rule = NULL;
	n->terminal = NULL;
	n->guard = false;
	n->hard = false;
	n->prev = NULL;
	n->next = NULL;
	return n;
}

cff_SubrRule *cff_new_Rule() {
	cff_SubrRule *r;
	NEW_CLEAN(r);
	r->refcount = 0;
	r->guard = cff_new_Node();
	r->guard->prev = r->guard;
	r->guard->next = r->guard;
	r->guard->terminal = 0;
	r->guard->guard = true;
	r->guard->rule = r;
	return r;
}

cff_SubrGraph *cff_new_Graph() {
	cff_SubrGraph *g;
	NEW_CLEAN(g);
	g->root = cff_new_Rule();
	cff_SubrNode *start = cff_new_Node();
	caryll_buffer *blob = bufnew();
	cff_mergeCS2Operator(blob, op_endchar);
	start->terminal = blob;
	insertNodeAfter(g->root->guard, start);
	return g;
}

cff_SubrNode *lastNodeOf(cff_SubrRule *r) {
	return r->guard->prev;
}

cff_SubrNode *copyNode(cff_SubrNode *n) {
	cff_SubrNode *m = cff_new_Node();
	if (n->rule) {
		m->rule = n->rule;
		m->rule->refcount += 1;
	} else {
		m->terminal = bufnew();
		bufwrite_buf(m->terminal, n->terminal);
	}
	return m;
}

// checkNode: check whether node N is shrinkable
bool checkDoubletMatch(cff_SubrGraph *g, cff_SubrNode *n);

static void unlinkNode(cff_SubrGraph *g, cff_SubrNode *a) {
	if (a->hard || a->guard) return;
	size_t len;
	uint8_t *key = getDoubletHashKey(a, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (di && di->start == a) { HASH_DEL(g->diagramIndex, di); }
	FREE(key);
	key = getSingletHashKey(a, &len);
	di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (di && di->start == a) { HASH_DEL(g->diagramIndex, di); }
	return;
}

static void addDoublet(cff_SubrGraph *g, cff_SubrNode *n) {
	if (!n || !n->next || n->guard || n->hard || n->next->hard || n->next->guard) return;
	size_t len;
	uint8_t *key = getDoubletHashKey(n, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (!di) {
		NEW(di);
		di->arity = 2;
		di->key = key;
		di->start = n;
		HASH_ADD_KEYPTR(hh, g->diagramIndex, key, len, di);
	} else {
		di->start = n;
		FREE(key);
	}
}
static void addSinglet(cff_SubrGraph *g, cff_SubrNode *n) {
	if (!n || n->guard || n->hard) return;
	size_t len;
	uint8_t *key = getSingletHashKey(n, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (!di) {
		NEW(di);
		di->arity = 1;
		di->key = key;
		di->start = n;
		HASH_ADD_KEYPTR(hh, g->diagramIndex, key, len, di);
	} else {
		di->start = n;
		FREE(key);
	}
}
static void linkNode(cff_SubrGraph *g, cff_SubrNode *n) {
	addSinglet(g, n);
	addDoublet(g, n);
}

bool identNode(cff_SubrNode *m, cff_SubrNode *n) {
	if (m->rule)
		return (m->rule == n->rule);
	else if (n->rule)
		return false;
	else
		return (m->terminal->size == n->terminal->size &&
		        strncmp((char *)m->terminal->data, (char *)n->terminal->data, m->terminal->size) == 0);
}
void joinNodes(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n) {
	if (m->next) {
		unlinkNode(g, m);
		if (n->prev && n->next && identNode(n->prev, n) && identNode(n, n->next)) { addDoublet(g, n); }
		if (m->prev && m->next && identNode(m->prev, m) && identNode(m, m->next)) { addDoublet(g, m->prev); }
	}
	m->next = n;
	n->prev = m;
}
void xInsertNodeAfter(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n) {
	joinNodes(g, n, m->next);
	joinNodes(g, m, n);
}
void removeNodeFromGraph(cff_SubrGraph *g, cff_SubrNode *a) {
	joinNodes(g, a->prev, a->next);
	if (!a->guard) {
		unlinkNode(g, a);
		delete_Node(a);
	}
}

void expandCall(cff_SubrGraph *g, cff_SubrNode *a) {
	cff_SubrNode *aprev = a->prev;
	cff_SubrNode *anext = a->next;
	cff_SubrNode *r1 = a->rule->guard->next;
	cff_SubrNode *r2 = a->rule->guard->prev;
	if (aprev == anext || r1 == r2) return;
	// We should move out [a, a'] from g's diagramIndex
	unlinkNode(g, a);

	joinNodes(g, aprev, r1);
	joinNodes(g, r2, anext);
	addDoublet(g, r2);
}

void substituteDoubletWithRule(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrRule *r) {
	cff_SubrNode *prev = m->prev;
	removeNodeFromGraph(g, prev->next);
	removeNodeFromGraph(g, prev->next);
	cff_SubrNode *invoke = cff_new_Node();
	invoke->rule = r;
	invoke->rule->refcount += 1;
	xInsertNodeAfter(g, prev, invoke);
	addDoublet(g, prev);
	addDoublet(g, invoke);
	if (!checkDoubletMatch(g, prev)) { checkDoubletMatch(g, prev->next); }
}
void substituteSingletWithRule(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrRule *r) {
	cff_SubrNode *prev = m->prev;
	removeNodeFromGraph(g, prev->next);
	cff_SubrNode *invoke = cff_new_Node();
	invoke->rule = r;
	invoke->rule->refcount += 1;
	xInsertNodeAfter(g, prev, invoke);
	addDoublet(g, prev);
	addDoublet(g, invoke);
}

void processMatchDoublet(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n) {
	cff_SubrRule *rule = NULL;
	if (m->prev->guard && m->next->next->guard) {
		// The match [m, m'] is a rule's full content
		rule = m->prev->rule;
		substituteDoubletWithRule(g, n, rule);
	} else {
		// Create a new rule
		rule = cff_new_Rule();
		rule->uniqueIndex = g->totalRules;
		g->totalRules += 1;
		xInsertNodeAfter(g, lastNodeOf(rule), copyNode(m));
		xInsertNodeAfter(g, lastNodeOf(rule), copyNode(m->next));
		substituteDoubletWithRule(g, m, rule);
		substituteDoubletWithRule(g, n, rule);
		addDoublet(g, rule->guard->next);
	}

	if (rule->guard->next->rule && rule->guard->next->rule->refcount == 1) {
		// The rule is shrinkable.
		expandCall(g, rule->guard->next);
	}
}
void processMatchSinglet(cff_SubrGraph *g, cff_SubrNode *m, cff_SubrNode *n) {
	cff_SubrRule *rule = NULL;
	if (m->prev->guard && m->next->guard) {
		// The match [m] is a rule's full content
		rule = m->prev->rule;
		substituteSingletWithRule(g, n, rule);
	} else {
		// Create a new rule
		rule = cff_new_Rule();
		rule->uniqueIndex = g->totalRules;
		g->totalRules += 1;
		xInsertNodeAfter(g, lastNodeOf(rule), copyNode(m));
		substituteSingletWithRule(g, m, rule);
		substituteSingletWithRule(g, n, rule);
		addSinglet(g, rule->guard->next);
	}
}

bool checkDoubletMatch(cff_SubrGraph *g, cff_SubrNode *n) {
	if (n->guard || n->next->guard || n->hard || n->next->hard) return false;
	// printf("test "), printNode(n, false), printNode(n->next, true);
	size_t len;
	uint8_t *key = getDoubletHashKey(n, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (!di) {
		NEW(di);
		di->arity = 2;
		di->key = key;
		di->start = n;
		HASH_ADD_KEYPTR(hh, g->diagramIndex, key, len, di);
		return false;
	} else if (di->arity == 2 && di->start != n && !di->start->guard && !di->start->next->guard) {
		FREE(key);
		processMatchDoublet(g, di->start, n);
		return true;
	} else {
		FREE(key);
		return true;
	}
}

bool checkSingletMatch(cff_SubrGraph *g, cff_SubrNode *n) {
	if (n->guard || n->hard) return false;
	size_t len;
	uint8_t *key = getSingletHashKey(n, &len);
	cff_SubrDiagramIndex *di = NULL;
	HASH_FIND(hh, g->diagramIndex, key, len, di);
	if (!di) {
		NEW(di);
		di->arity = 1;
		di->key = key;
		di->start = n;
		HASH_ADD_KEYPTR(hh, g->diagramIndex, key, len, di);
		return false;
	} else if (di->arity == 1 && !(n->hard) && di->start != n && !di->start->guard) {
		FREE(key);
		processMatchSinglet(g, di->start, n);
		return true;
	} else {
		FREE(key);
		return false;
	}
}

void appendNodeToGraph(cff_SubrGraph *g, cff_SubrNode *n) {
	cff_SubrNode *last = lastNodeOf(g->root);
	insertNodeAfter(last, n);
	//	if (buflen(n->terminal) > 8) checkSingletMatch(g, n);
	checkDoubletMatch(g, last);
}

void cff_insertILToGraph(cff_SubrGraph *g, cff_CharstringIL *il) {
	caryll_buffer *blob = bufnew();
	bool hard = true;
	bool flush = false;
	for (uint16_t j = 0; j < il->length; j++) {
		switch (il->instr[j].type) {
			case IL_ITEM_OPERAND: {
				if (flush) {
					cff_SubrNode *n = cff_new_Node();
					n->rule = NULL;
					n->terminal = blob;
					n->hard = hard;
					appendNodeToGraph(g, n);
					blob = bufnew();
					flush = false;
				}
				cff_mergeCS2Operand(blob, il->instr[j].d);
				break;
			}
			case IL_ITEM_PROGID: {
				cff_mergeCS2Operand(blob, il->instr[j].i);
				flush = true;
				break;
			}
			case IL_ITEM_OPERATOR: {
				cff_mergeCS2Operator(blob, il->instr[j].i);
				if (il->instr[j].i == op_rmoveto || il->instr[j].i == op_hmoveto || il->instr[j].i == op_vmoveto) {
					hard = false;
				}
				flush = true;
				break;
			}
			case IL_ITEM_SPECIAL: {
				cff_mergeCS2Special(blob, il->instr[j].i);
				flush = true;
				break;
			}
			default:
				break;
		}
	}
	if (blob->size) {
		cff_SubrNode *n = cff_new_Node();
		n->rule = NULL;
		n->terminal = blob;
		n->hard = hard;
		appendNodeToGraph(g, n);
		blob = bufnew();
		cff_mergeCS2Operator(blob, op_endchar);
		n = cff_new_Node();
		n->rule = NULL;
		n->terminal = blob;
		n->hard = true;
		appendNodeToGraph(g, n);
	}
}
