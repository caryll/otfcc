#include "cff-value.h"

double cffnum(CFF_Value val) {
	if (val.t == CFF_INTEGER) return val.i;
	if (val.t == CFF_DOUBLE) return val.d;
	return 0;
}
