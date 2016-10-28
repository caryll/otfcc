#include <math.h>
#include "otfcc/primitives.h"

// f2dot14 type
double otfcc_from_f2dot14(const f2dot14 x) {
	return x / 16384.0;
}
int16_t otfcc_to_f2dot14(const double x) {
	return round(x * 16384.0);
}

// F16.16 (fixed) type
double otfcc_from_fixed(const f16dot16 x) {
	return x / 65536.0;
}
f16dot16 otfcc_to_fixed(const double x) {
	return round(x * 65536.0);
}
