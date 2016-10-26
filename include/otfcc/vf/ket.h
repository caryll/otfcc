#ifndef CARYLL_VF_KET_H
#define CARYLL_VF_KET_H

#include "caryll/ownership.h"
#include "otfcc/primitives.h"

// vf_Ket type
typedef struct vf_KetMaster {
	OWNING pos_t *start; // arity === implicit quantity of axes; null for all -1
	OWNING pos_t *end;   // null for all 1
	OWNING pos_t *peak;
} vf_KetMaster;
typedef struct vf_Ket {
	shapeid_t nMasters;
	shapeid_t nAxes;
	OWNING vf_KetMaster *masters;
} vf_Ket;

#endif
