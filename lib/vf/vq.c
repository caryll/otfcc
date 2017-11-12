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
    caryll_VectorImplAssignments(VV, pos_t, vq_iPosT),
    .neutral = createNeutralVV,
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
		case VQ_DELTA:
			fprintf(stderr, "{%g%s", x.val.delta.quantity, x.val.delta.touched ? " " : "* ");
			foreach (vq_AxisSpan *s, x.val.delta.region) {
				fprintf(stderr, "[%g %g %g]", s->start, s->peak, s->end);
			}
			fprintf(stderr, "}\n");
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
    .createStill = vqsCreateStill,
    .createDelta = vqsCreateDelta,
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

// Module
static void vqInplaceScale(MODIFY VQ *a, pos_t b) {
	a->kernel *= b;
	for (size_t j = 0; j < a->shift.length; j++) {
		vq_Segment *s = &a->shift.items[j];
		switch (s->type) {
			case VQ_STILL:
				s->val.still *= b;
				break;
			case VQ_DELTA:
				s->val.delta.quantity *= b;
				break;
		}
	}
}

// Group
static void vqInplaceNegate(MODIFY VQ *a) {
	vqInplaceScale(a, -1);
}

caryll_GroupFns(VQ, vqInplaceNegate);
caryll_ModuleFns(VQ, pos_t, vqInplaceScale);

// Ord
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

// pointLinearTfm
static VQ vqPointLinearTfm(const VQ ax, pos_t a, const VQ x, pos_t b, const VQ y) {
	VQ targetX = iVQ.dup(ax);
	iVQ.inplacePlusScale(&targetX, a, x);
	iVQ.inplacePlusScale(&targetX, b, y);
	return targetX;
}

caryll_VectorInterfaceTypeName(VQ) iVQ = {
    caryll_standardValTypeMethods(VQ),
    .getStill = vqGetStill,
    .createStill = vqCreateStill,
    caryll_MonoidAssigns(VQ),          // Monoid
    caryll_GroupAssigns(VQ),           // Group
    caryll_ModuleAssigns(VQ),          // Module
    caryll_OrdEqAssigns(VQ),           // Eq-Ord
    caryll_ShowAssigns(VQ),            // Show
    .pointLinearTfm = vqPointLinearTfm // pointLinearTfm
};

// JSON conversion functions
// dump
json_value *json_new_VQAxisSpan(const vq_AxisSpan *s) {
	if ((s->peak > 0 && s->start == 0 && s->end == 1) ||
	    (s->peak < 0 && s->start == -1 && s->end == 0)) {
		return json_new_position(s->peak);
	} else {
		json_value *a = json_object_new(3);
		json_object_push(a, "start", json_new_position(s->start));
		json_object_push(a, "peak", json_new_position(s->peak));
		json_object_push(a, "end", json_new_position(s->end));
		return a;
	}
}
json_value *json_new_VQRegion(const vq_Region *rs, const vf_Axes *axes) {
	if (axes && axes->length == rs->length) {
		json_value *r = json_object_new(rs->length);
		for (size_t j = 0; j < rs->length; j++) {
			json_object_push_tag(r, axes->items[j].tag, json_new_VQAxisSpan(&rs->items[j]));
		}
		return r;
	} else {
		json_value *r = json_array_new(rs->length);
		for (size_t j = 0; j < rs->length; j++) {
			json_array_push(r, json_new_VQAxisSpan(&rs->items[j]));
		}
		return r;
	}
}
json_value *json_new_VQSegment(const vq_Segment *s, const vf_Axes *axes) {
	switch (s->type) {
		case VQ_STILL:;
			return json_new_position(s->val.still);
		case VQ_DELTA:;
			json_value *d = json_object_new(3);
			json_object_push(d, "delta", json_new_position(s->val.delta.quantity));
			if (!s->val.delta.touched) {
				json_object_push(d, "implicit", json_boolean_new(!s->val.delta.touched));
			}
			json_object_push(d, "over", json_new_VQRegion(&s->val.delta.region, axes));
			return d;
		default:;
			return json_integer_new(0);
	}
}
json_value *json_new_VQ(const VQ z, const vf_Axes *axes) {
	if (!z.shift.length) {
		return preserialize(json_new_position(iVQ.getStill(z)));
	} else {
		json_value *a = json_array_new(z.shift.length + 1);
		json_array_push(a, json_new_position(z.kernel));
		for (size_t j = 0; j < z.shift.length; j++) {
			json_array_push(a, json_new_VQSegment(&z.shift.items[j], axes));
		}
		return preserialize(a);
	}
}

json_value *json_new_VV(const VV x, const vf_Axes *axes) {
	if (axes && axes->length == x.length) {
		json_value *_coord = json_object_new(axes->length);
		for (size_t m = 0; m < x.length; m++) {
			vf_Axis *axis = &axes->items[m];
			char tag[4] = {(axis->tag & 0xff000000) >> 24, (axis->tag & 0xff0000) >> 16,
			               (axis->tag & 0xff00) >> 8, (axis->tag & 0xff)};
			json_object_push_length(_coord, 4, tag, json_new_position(x.items[m]));
		}
		return preserialize(_coord);
	} else {
		json_value *_coord = json_array_new(x.length);
		for (size_t m = 0; m < x.length; m++) {
			json_array_push(_coord, json_new_position(x.items[m]));
		}
		return preserialize(_coord);
	}
}
json_value *json_new_VVp(const VV *x, const vf_Axes *axes) {
	if (axes && axes->length == x->length) {
		json_value *_coord = json_object_new(axes->length);
		for (size_t m = 0; m < x->length; m++) {
			vf_Axis *axis = &axes->items[m];
			char tag[4] = {(axis->tag & 0xff000000) >> 24, (axis->tag & 0xff0000) >> 16,
			               (axis->tag & 0xff00) >> 8, (axis->tag & 0xff)};
			json_object_push_length(_coord, 4, tag, json_new_position(x->items[m]));
		}
		return preserialize(_coord);

	} else {
		json_value *_coord = json_array_new(x->length);
		for (size_t m = 0; m < x->length; m++) {
			json_array_push(_coord, json_new_position(x->items[m]));
		}
		return preserialize(_coord);
	}
}
// parse
VQ json_vqOf(const json_value *cv, const vf_Axes *axes) {
	return iVQ.createStill(json_numof(cv));
}
