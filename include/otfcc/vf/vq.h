#ifndef CARYLL_VF_FUNCTIONAL_H
#define CARYLL_VF_FUNCTIONAL_H

#include "caryll/ownership.h"
#include "caryll/element.h"
#include "caryll/vector.h"
#include "otfcc/primitives.h"
#include "otfcc/handle.h"

typedef struct {
	otfcc_AxisHandle axis;
	pos_t start;
	pos_t peak;
	pos_t end;
} vq_AxisSpan;
extern caryll_ElementInterface(vq_AxisSpan) vq_iAxisSpan;
typedef caryll_Vector(vq_AxisSpan) vq_Region;
extern caryll_VectorInterface(vq_Region, vq_AxisSpan) vq_iRegion;

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

extern caryll_ElementInterface(vq_Segment) vq_iSegment;
typedef caryll_Vector(vq_Segment) VQ;
// extern caryll_VectorInterface(VQ, vq_Segment) iVQ;
extern caryll_VectorInterfaceTypeName(VQ) {
	caryll_VectorInterfaceTrait(VQ, vq_Segment);
	pos_t (*getNeutral)(const VQ *v);
}
iVQ;
#endif
