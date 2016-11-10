#ifndef CARYLL_VF_FUNCTIONAL_H
#define CARYLL_VF_FUNCTIONAL_H

#include "caryll/ownership.h"
#include "otfcc/primitives.h"

// vf_Functional type
typedef enum {
	vf_null = 0,     // zero scalar
	vf_scalar = 1,   // simple scalar
	vf_plus = 2,     // <plus z1 z2 | x> = <z1 | x> + <z2 | x>
	vf_minus = 3,    // <minus z1 z2 | x> = <z1 | x> - <z2 | x>
	vf_multiply = 4, // <multiply z1 z2 | x> = <z1 | s> <z2 | x>
	vf_gxblend = 5   // <gxblend d1 d2 ... dn | w1 w2 ... wn> = <d1 | w> * w1 + ... + <dn | w> * wn;
} VF_OPERATOR;

typedef struct vf_Functional {
	VF_OPERATOR car;
	union {
		pos_t scalar;
		struct {
			shapeid_t arity;
			OWNING struct vf_Functional *cdr;
		};
	};
} vf_Functional;

struct vf_IFunctional {
	void (*dispose)(MOVE vf_Functional *form);
	void (*destroy)(MOVE vf_Functional *form);
	vf_Functional (*scalar)(pos_t scalar);
	vf_Functional (*add)(MOVE vf_Functional a, MOVE vf_Functional b);
	vf_Functional (*minus)(MOVE vf_Functional a, MOVE vf_Functional b);
	vf_Functional (*multi)(MOVE vf_Functional a, MOVE vf_Functional b);
	pos_t (*zero)(OBSERVE vf_Functional a);
	pos_t (*base)(OBSERVE vf_Functional a, shapeid_t n);
	vf_Functional (*gxCanonical)(OBSERVE vf_Functional a, shapeid_t n);
};

extern const struct vf_IFunctional vf_iFunctional;

#endif
