#ifndef CARYLL_VF_FUNCTIONAL_H
#define CARYLL_VF_FUNCTIONAL_H
#include "support/util.h"

// vf_Functional type
typedef enum { vf_null = 0, vf_scalar = 1, vf_plus = 2, vf_minus = 3, vf_multply = 4, vf_gxblend = 5 } VF_OPERATOR;

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
