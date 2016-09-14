#ifndef CARYLL_cff_SUBR_H
#define CARYLL_cff_SUBR_H

#include "libcff.h"
#include "charstring-il.h"

typedef struct __cff_SubrRule cff_SubrRule;
typedef struct __cff_SubrNode cff_SubrNode;

struct __cff_SubrNode {
	caryll_buffer *terminal;
	cff_SubrRule *rule;
	cff_SubrNode *prev;
	cff_SubrNode *next;
	bool hard;
	bool guard;
};

struct __cff_SubrRule {
	bool printed;
	uint32_t uniqueIndex;
	uint16_t cffIndex;
	uint32_t refcount;
	cff_SubrNode *guard;
};

typedef struct {
	uint8_t arity;
	uint8_t *key;
	cff_SubrNode *start;
	UT_hash_handle hh;
} cff_SubrDiagramIndex;

typedef struct {
	cff_SubrRule *root;
	cff_SubrDiagramIndex *diagramIndex;
	uint32_t totalRules;
} cff_SubrGraph;

cff_SubrGraph *cff_new_Graph();
void cff_insertILToGraph(cff_SubrGraph *g, cff_CharstringIL *il);
void printRule(cff_SubrRule *r);
#endif
