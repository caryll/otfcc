#ifndef CARYLL_VF_FUNCTIONAL_H
#define CARYLL_VF_FUNCTIONAL_H

#include "caryll/ownership.h"
#include "caryll/element.h"
#include "caryll/vector.h"
#include "otfcc/primitives.h"
#include "otfcc/handle.h"

extern caryll_ValElementInterface(pos_t) vq_iPosT;
typedef caryll_Vector(pos_t) VV;
extern caryll_VectorInterfaceTypeName(VV) {
	caryll_VectorInterfaceTrait(VV, pos_t);
	// Monoid instances
	VV (*neutral)(tableid_t dimensions);
}
iVV;
// extern caryll_VectorInterface(VV, pos_t) iVV;

typedef struct {
	pos_t start;
	pos_t peak;
	pos_t end;
} vq_AxisSpan;
extern caryll_ElementInterface(vq_AxisSpan) vq_iAxisSpan;
typedef caryll_Vector(vq_AxisSpan) vq_Region;
extern caryll_VectorInterfaceTypeName(vq_Region) {
	caryll_VectorInterfaceTrait(vq_Region, vq_AxisSpan);
	caryll_Ord(vq_Region);
	pos_t (*getWeight)(const vq_Region *r, const VV *vv);
}
vq_iRegion;

typedef enum { VQ_STILL = 0, VQ_DELTA = 1 } VQSegType;
typedef struct {
	VQSegType type;
	union {
		pos_t still;
		struct {
			pos_t quantity;
			vq_Region region;
		} delta;
	} val;
} vq_Segment;

extern caryll_ElementInterfaceOf(vq_Segment) {
	caryll_VT(vq_Segment);
	caryll_Show(vq_Segment);
	caryll_Ord(vq_Segment);
	vq_Segment (*createStill)(pos_t x);
	vq_Segment (*createDelta)(pos_t delta, MOVE vq_Region region);
}
vq_iSegment;
typedef caryll_Vector(vq_Segment) vq_SegList;
extern caryll_VectorInterface(vq_SegList, vq_Segment) vq_iSegList;

// VQ
typedef struct {
	pos_t kernel;
	vq_SegList shift;
} VQ;
extern caryll_VectorInterfaceTypeName(VQ) {
	caryll_VT(VQ);
	caryll_Module(VQ, scale_t); // VQ forms a module (vector space)
	caryll_Ord(VQ);             // VQs are comparable
	caryll_Show(VQ);
	// Getting still
	pos_t (*getStill)(const VQ v);
	// Creating still
	VQ (*createStill)(pos_t x);

	// util functions
	// point linear transform
	VQ (*pointLinearTfm)(const VQ ax, pos_t a, const VQ x, pos_t b, const VQ y);
}
iVQ;
#endif
