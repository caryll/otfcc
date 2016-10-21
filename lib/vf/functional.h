#ifndef CARYLL_VF_FUNCTIONAL_H
#define CARYLL_VF_FUNCTIONAL_H
#include "support/util.h"

// vf_Functional type
typedef enum {
	vf_null = 0,     // zero scalar
	vf_scalar = 1,   // simple scalar
	vf_plus = 2,     // <plus z1 z2 | x> = <z1 | x> + <z2 | x>
	vf_minus = 3,    // <minus z1 z2 | x> = <z1 | x> - <z2 | x>
	vf_multiply = 4, // <multiply s z2 | x> = s <z2 | x>
	vf_gxblend = 5   // <gxblend x d1 d2 ... dn | w1 w2 ... wn> = x + <d1 | w> * w1 + ... + <dn | w> * wn;
} VF_OPERATOR;

typedef struct vf_Functional {
	VF_OPERATOR car;
	union {
		pos_t scalar;
		struct {
			shapeid_t arity;
			struct vf_Functional *cdr;
		};
	};
} vf_Functional;

vf_Functional *vf_Functional_shrink(MOVE vf_Functional *form);

#endif
