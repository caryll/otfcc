#include "otfcc/vf/vq.h"
#include "support/util.h"

static INLINE void initAxisSpan(vq_AxisSpan *as) {
	Handle.init(&as->axis);
}
static INLINE void copyAxisSpan(vq_AxisSpan *dst, const vq_AxisSpan *src) {
	Handle.copy(&dst->axis, &src->axis);
	dst->start = src->start;
	dst->peak = src->peak;
	dst->end = src->end;
}
static INLINE void disposeAxisSpan(vq_AxisSpan *as) {
	Handle.dispose(&as->axis);
}
caryll_standardType(vq_AxisSpan, vq_iAxisSpan, initAxisSpan, copyAxisSpan, disposeAxisSpan);
caryll_standardVectorImpl(vq_Region, vq_AxisSpan, vq_iAxisSpan, vq_iRegion);

static INLINE void initVQSegment(vq_Segment *vqs) {
	vqs->type = VQ_STILL;
	vqs->val.still = 0;
}
static INLINE void copyVQSegment(vq_Segment *dst, const vq_Segment *src) {
	dst->type = src->type;
	switch (dst->type) {
		case VQ_STILL:
			dst->val.still = src->val.still;
			break;
		case VQ_DELTA:
			dst->val.delta.quantity = src->val.delta.quantity;
			vq_iRegion.copy(&dst->val.delta.region, &src->val.delta.region);
	}
}
static INLINE void disposeVQSegment(vq_Segment *vqs) {
	switch (vqs->type) {
		case VQ_DELTA:
			vq_iRegion.dispose(&vqs->val.delta.region);
		default:;
	}
	initVQSegment(vqs);
}

caryll_standardType(vq_Segment, vq_iSegment, initVQSegment, copyVQSegment, disposeVQSegment);
caryll_VectorImplFunctions(VQ, vq_Segment, vq_iSegment);

static pos_t vqGetNeutral(const VQ *v) {
	pos_t result = 0;
	for (size_t j = 0; j < v->length; j++) {
		switch (v->items[j].type) {
			case VQ_STILL:
				result += v->items[j].val.still;
			default:;
		}
	}
	return result;
}

caryll_VectorInterfaceTypeName(VQ) iVQ = {caryll_VectorImplAssignments(VQ, vq_Segment, vq_iSegment),
                                          .getNeutral = vqGetNeutral};
