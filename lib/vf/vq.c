#include "otfcc/vf/vq.h"
#include "support/util.h"

// Variation vector
caryll_standardValType(pos_t, vq_iPosT);
caryll_VectorImplFunctions(VV, pos_t, vq_iPosT);

static VV createNeutralVV(tableid_t dimensions) {
	VV vv;
	iVV.initN(&vv, dimensions);
	for (tableid_t j = 0; j < dimensions; j++) {
		vv.items[j] = 0;
	}
	return vv;
};
caryll_VectorInterfaceTypeName(VV) iVV = {
    caryll_VectorImplAssignments(VV, pos_t, vq_iPosT), .neutral = createNeutralVV,
};

// Axis span and region
static INLINE void initAxisSpan(vq_AxisSpan *as) {
	as->start = 0;
	as->peak = 0;
	as->end = 0;
}
static INLINE void copyAxisSpan(vq_AxisSpan *dst, const vq_AxisSpan *src) {
	dst->start = src->start;
	dst->peak = src->peak;
	dst->end = src->end;
}
static INLINE void disposeAxisSpan(vq_AxisSpan *as) {}
caryll_standardType(vq_AxisSpan, vq_iAxisSpan, initAxisSpan, copyAxisSpan, disposeAxisSpan);

static pos_t INLINE weightAxisRegion(const vq_AxisSpan *as, const pos_t x) {
	const pos_t a = as->start;
	const pos_t p = as->peak;
	const pos_t z = as->end;
	if (a > p || p > z) {
		return 1;
	} else if (a < 0 && z > 0 && p != 0) {
		return 1;
	} else if (p == 0) {
		return 1;
	} else if (x < a || x > z) {
		return 0;
	} else if (x == p) {
		return 1;
	} else if (x < p) {
		return (x - a) / (p - a);
	} else {
		return (z - x) / (z - p);
	}
}
static pos_t vqRegionGetWeight(const vq_Region *r, const VV *v) {
	pos_t w = 1;
	for (size_t j = 0; j < r->length && v->length; j++) {
		w *= weightAxisRegion(&r->items[j], v->items[j]);
	}
	return w;
}

static int vqrCompare(const vq_Region a, const vq_Region b) {
	if (a.length < b.length) return -1;
	if (a.length > b.length) return 1;
	for (size_t j = 0; j < a.length; j++) {
		if (a.items[j].start < b.items[j].start) return -1;
		if (a.items[j].start > b.items[j].start) return 1;
		if (a.items[j].peak < b.items[j].peak) return -1;
		if (a.items[j].peak > b.items[j].peak) return 1;
		if (a.items[j].end < b.items[j].end) return -1;
		if (a.items[j].end > b.items[j].end) return 1;
	}
	return 0;
}

caryll_VectorImplFunctions(vq_Region, vq_AxisSpan, vq_iAxisSpan);
caryll_OrdEqFns(vq_Region, vqrCompare);

caryll_VectorInterfaceTypeName(vq_Region) vq_iRegion = {
    caryll_VectorImplAssignments(vq_Region, vq_AxisSpan, vq_iAxisSpan),
    caryll_OrdEqAssigns(vq_Region), // Ord
    .getWeight = vqRegionGetWeight,
};

// VQS
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
			break;
		default:;
	}
	initVQSegment(vqs);
}

caryll_standardValTypeFn(vq_Segment, initVQSegment, copyVQSegment, disposeVQSegment);
static vq_Segment vqsCreateStill(pos_t x) {
	vq_Segment vqs;
	vq_iSegment.init(&vqs);
	vqs.val.still = x;
	return vqs;
}
static vq_Segment vqsCreateDelta(pos_t delta, MOVE vq_Region region) {
	vq_Segment vqs;
	vq_iSegment.init(&vqs);
	vqs.type = VQ_DELTA;
	vqs.val.delta.quantity = delta;
	vqs.val.delta.region = region;
	return vqs;
}

static int vqsCompare(const vq_Segment a, const vq_Segment b) {
	if (a.type < b.type) return -1;
	if (a.type > b.type) return 1;
	switch (a.type) {
		case VQ_STILL: {
			if (a.val.still < b.val.still) return -1;
			if (a.val.still > b.val.still) return 1;
			return 0;
		}
		case VQ_DELTA: {
			int vqrc = vqrCompare(a.val.delta.region, b.val.delta.region);
			if (vqrc) return vqrc;
			if (a.val.delta.quantity < b.val.delta.quantity) return -1;
			if (a.val.delta.quantity > b.val.delta.quantity) return 1;
			return 0;
		}
	}
}
caryll_OrdEqFns(vq_Segment, vqsCompare);
static void showVQS(const vq_Segment x) {
	switch (x.type) {
		case VQ_STILL:
			fprintf(stderr, "%g", x.val.still);
			return;
		default:;
	}
}
caryll_ShowFns(vq_Segment, showVQS);
caryll_ElementInterfaceOf(vq_Segment) vq_iSegment = {
    caryll_standardValTypeMethods(vq_Segment),
    caryll_OrdEqAssigns(vq_Segment), // Ord and Eq
    caryll_ShowAssigns(vq_Segment),  // Show
    // creating instances
    .createStill = vqsCreateStill, .createDelta = vqsCreateDelta,
};

caryll_standardVectorImpl(vq_SegList, vq_Segment, vq_iSegment, vq_iSegList);

// Monoid

static INLINE void vqInit(VQ *a) {
	a->kernel = 0;
	vq_iSegList.init(&a->shift);
}
static INLINE void vqCopy(VQ *a, const VQ *b) {
	a->kernel = b->kernel;
	vq_iSegList.copy(&a->shift, &b->shift);
}
static INLINE void vqDispose(VQ *a) {
	a->kernel = 0;
	vq_iSegList.dispose(&a->shift);
}

caryll_standardValTypeFn(VQ, vqInit, vqCopy, vqDispose);
static VQ vqNeutral() {
	return iVQ.createStill(0);
}
static bool vqsCompatible(const vq_Segment a, const vq_Segment b) {
	if (a.type != b.type) return false;
	switch (a.type) {
		case VQ_STILL:
			return true;
		case VQ_DELTA:
			return vq_iRegion.equal(a.val.delta.region, b.val.delta.region);
	}
}
static void simplifyVq(MODIFY VQ *x) {
	if (!x->shift.length) return;
	vq_iSegList.sort(&x->shift, vq_iSegment.compareRef);
	size_t k = 0;
	for (size_t j = 1; j < x->shift.length; j++) {
		if (vqsCompatible(x->shift.items[k], x->shift.items[j])) {
			switch (x->shift.items[k].type) {
				case VQ_STILL:
					x->shift.items[k].val.still += x->shift.items[j].val.still;
					break;
				case VQ_DELTA:
					x->shift.items[k].val.delta.quantity += x->shift.items[j].val.delta.quantity;
					break;
			}
			vq_iSegment.dispose(&x->shift.items[j]);
		} else {
			x->shift.items[k] = x->shift.items[j];
			k++;
		}
	}
	x->shift.length = k + 1;
}
static void vqInplacePlus(MODIFY VQ *a, const VQ b) {
	a->kernel += b.kernel;
	for (size_t p = 0; p < b.shift.length; p++) {
		vq_Segment *k = &b.shift.items[p];
		if (k->type == VQ_STILL) {
			a->kernel += k->val.still;
		} else {
			vq_Segment s;
			vq_iSegment.copy(&s, k);
			vq_iSegList.push(&a->shift, s);
		}
	}
	simplifyVq(a);
}

caryll_MonoidFns(VQ, vqNeutral, vqInplacePlus);

// Group
static void vqInplaceNegate(MODIFY VQ *a) {
	a->kernel *= -1;
	for (size_t j = 0; j < a->shift.length; j++) {
		vq_Segment *s = &a->shift.items[j];
		switch (s->type) {
			case VQ_STILL:
				s->val.still *= -1;
				break;
			case VQ_DELTA:
				s->val.delta.quantity *= -1;
				break;
		}
	}
}

caryll_GroupFns(VQ, vqInplaceNegate);

// Eq
static int vqCompare(const VQ a, const VQ b) {
	if (a.shift.length < b.shift.length) return -1;
	if (a.shift.length > b.shift.length) return 1;
	for (size_t j = 0; j < a.shift.length; j++) {
		int cr = vqsCompare(a.shift.items[j], b.shift.items[j]);
		if (cr) return cr;
	}
	return a.kernel - b.kernel;
}
caryll_OrdEqFns(VQ, vqCompare);

// Show
static void showVQ(const VQ x) {
	fprintf(stderr, "%g + {", x.kernel);
	for (size_t j = 0; j < x.shift.length; j++) {
		if (j) fprintf(stderr, " ");
		vq_iSegment.show(x.shift.items[j]);
	}
	fprintf(stderr, "}\n");
}
caryll_ShowFns(VQ, showVQ);

// Still instances
static pos_t vqGetStill(const VQ v) {
	pos_t result = v.kernel;
	for (size_t j = 0; j < v.shift.length; j++) {
		switch (v.shift.items[j].type) {
			case VQ_STILL:
				result += v.shift.items[j].val.still;
			default:;
		}
	}
	return result;
}
static VQ vqCreateStill(pos_t x) {
	VQ vq;
	iVQ.init(&vq);
	vq.kernel = x;
	return vq;
}

caryll_VectorInterfaceTypeName(VQ) iVQ = {
    caryll_standardValTypeMethods(VQ),
    .getStill = vqGetStill,
    .createStill = vqCreateStill,
    caryll_MonoidAssigns(VQ), // monoid
    caryll_GroupAssigns(VQ),  // group
    caryll_OrdEqAssigns(VQ),  // Ord
    caryll_ShowAssigns(VQ),   // show
};
