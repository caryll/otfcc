#ifndef CARYLL_cff_SUBR_H
#define CARYLL_cff_SUBR_H

#include "libcff.h"
#include "charstring-il.h"

typedef struct __cff_SubrRule cff_SubrRule;
typedef struct __cff_SubrNode cff_SubrNode;

struct __cff_SubrNode {
	caryll_Buffer *terminal;
	cff_SubrRule *rule;
	cff_SubrNode *prev;
	cff_SubrNode *next;
	bool hard;
	bool guard;
	bool last;
};

struct __cff_SubrRule {
	bool printed;
	bool numbered;
	uint32_t number;
	uint32_t height;
	uint32_t uniqueIndex;
	uint16_t cffIndex;
	uint32_t refcount;
	uint32_t effectiveLength;
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
	uint32_t totalCharStrings;
	bool doSubroutinize;
} cff_SubrGraph;

cff_SubrGraph *cff_new_Graph();
void cff_insertILToGraph(cff_SubrGraph *g, cff_CharstringIL *il);
void cff_ilGraphToBuffers(cff_SubrGraph *g, caryll_Buffer **s, caryll_Buffer **gs, caryll_Buffer **ls);
void cff_delete_Graph(cff_SubrGraph *g);
#endif
