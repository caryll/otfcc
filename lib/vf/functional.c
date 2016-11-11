#include "otfcc/vf/functional.h"
#include "support/util.h"

static void vfDispose(MOVE vf_Functional *form) {
	if (form->car > vf_scalar) {
		for (shapeid_t j = form->arity; j--;) {
			vfDispose(&form->cdr[j]);
		}
		FREE(form->cdr);
	}
}
caryll_standardValTypeFn(vf_Functional, vfDispose);

static vf_Functional vf_new_Functional_scalar(pos_t scalar) {
	vf_Functional f;
	f.car = vf_scalar;
	f.scalar = scalar;
	return f;
}
static vf_Functional vf_Functional_add(MOVE vf_Functional a, MOVE vf_Functional b) {
	vf_Functional f;
	f.car = vf_plus;
	f.arity = 2;
	NEW(f.cdr, 2);
	f.cdr[0] = a;
	f.cdr[1] = b;
	return f;
}
static vf_Functional vf_Functional_minus(MOVE vf_Functional a, MOVE vf_Functional b) {
	vf_Functional f;
	f.car = vf_minus;
	f.arity = 2;
	NEW(f.cdr, 2);
	f.cdr[0] = a;
	f.cdr[1] = b;
	return f;
}
static vf_Functional vf_Functional_multi(MOVE vf_Functional a, MOVE vf_Functional b) {
	vf_Functional f;
	f.car = vf_multiply;
	f.arity = 2;
	NEW(f.cdr, 2);
	f.cdr[0] = a;
	f.cdr[1] = b;
	return f;
}
static pos_t vf_Functional_zero(OBSERVE vf_Functional a) {
	switch (a.car) {
		case vf_null:
			return 0;
		case vf_scalar:
			return a.scalar;
		case vf_plus:
			return vf_Functional_zero(a.cdr[0]) + vf_Functional_zero(a.cdr[1]);
		case vf_minus:
			return vf_Functional_zero(a.cdr[0]) - vf_Functional_zero(a.cdr[1]);
		case vf_multiply:
			return vf_Functional_zero(a.cdr[0]) * vf_Functional_zero(a.cdr[1]);
		case vf_gxblend:
			return 0;
		default:
			return 0;
	}
}

static pos_t vf_Functional_base(OBSERVE vf_Functional a, shapeid_t n) {
	switch (a.car) {
		case vf_scalar:
			return a.scalar;
		case vf_plus:
			return vf_Functional_base(a.cdr[0], n) + vf_Functional_base(a.cdr[1], n);
		case vf_minus:
			return vf_Functional_base(a.cdr[0], n) - vf_Functional_base(a.cdr[1], n);
		case vf_multiply:
			return vf_Functional_base(a.cdr[0], n) * vf_Functional_base(a.cdr[1], n);
		case vf_gxblend:
			if (n < a.arity) {
				return vf_Functional_base(a.cdr[n], n);
			} else {
				return 0;
			}
		default:
			return 0;
	}
}

static vf_Functional vf_Functional_gxCanonical(OBSERVE vf_Functional a, shapeid_t n) {
	vf_Functional fz = vf_new_Functional_scalar(vf_Functional_zero(a));
	vf_Functional fb = {.car = vf_gxblend, .arity = n, .cdr = NULL};
	NEW(fb.cdr, n);
	for (tableid_t j = 0; j < n; j++) {
		fb.cdr[j] = vf_new_Functional_scalar(vf_Functional_base(a, n) - fz.scalar);
	}
	return vf_Functional_add(fz, fb);
}

const struct vf_IFunctional vf_iFunctional = {
    caryll_standardValTypeMethods(vf_Functional), // VT implementation
    .scalar = vf_new_Functional_scalar,
    .add = vf_Functional_add,
    .minus = vf_Functional_minus,
    .multi = vf_Functional_multi,
    .zero = vf_Functional_zero,
    .base = vf_Functional_base,
    .gxCanonical = vf_Functional_gxCanonical,
};
