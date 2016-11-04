#include "otfcc/vf/functional.h"
#include "support/util.h"

void vf_Functional_dispose(MOVE vf_Functional *form) {
	if (form && form->car > vf_scalar) {
		for (shapeid_t j = form->arity; j--;) {
			vf_Functional_dispose(&form->cdr[j]);
		}
		FREE(form->cdr);
	}
}
vf_Functional vf_new_Functional_scalar(pos_t scalar) {
	vf_Functional f;
	f.car = vf_scalar;
	f.scalar = scalar;
	return f;
}
vf_Functional vf_Functional_add(MOVE vf_Functional a, MOVE vf_Functional b) {
	vf_Functional f;
	f.car = vf_plus;
	f.arity = 2;
	NEW(f.cdr, 2);
	f.cdr[0] = a;
	f.cdr[1] = b;
	return f;
}
vf_Functional vf_Functional_minus(MOVE vf_Functional a, MOVE vf_Functional b) {
	vf_Functional f;
	f.car = vf_minus;
	f.arity = 2;
	NEW(f.cdr, 2);
	f.cdr[0] = a;
	f.cdr[1] = b;
	return f;
}
vf_Functional vf_Functional_multi(MOVE vf_Functional a, MOVE vf_Functional b) {
	vf_Functional f;
	f.car = vf_multiply;
	f.arity = 2;
	NEW(f.cdr, 2);
	f.cdr[0] = a;
	f.cdr[1] = b;
	return f;
}
pos_t vf_Functional_zero(OBSERVE vf_Functional a) {
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

pos_t vf_Functional_base(OBSERVE vf_Functional a, shapeid_t n) {
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

vf_Functional vf_Functional_gxCanonical(OBSERVE vf_Functional a, shapeid_t n) {
	vf_Functional fz = vf_new_Functional_scalar(vf_Functional_zero(a));
	vf_Functional fb = {.car = vf_gxblend, .arity = n, .cdr = NULL};
	NEW(fb.cdr, n);
	for (tableid_t j = 0; j < n; j++) {
		fb.cdr[j] = vf_new_Functional_scalar(vf_Functional_base(a, n) - fz.scalar);
	}
	return vf_Functional_add(fz, fb);
}

const struct otfcc_VFPackage otfcc_vfPackage = {
    .dispose = vf_Functional_dispose,
    .scalar = vf_new_Functional_scalar,
    .add = vf_Functional_add,
    .minus = vf_Functional_minus,
    .multi = vf_Functional_multi,
    .zero = vf_Functional_zero,
    .base = vf_Functional_base,
    .gxCanonical = vf_Functional_gxCanonical,
};
