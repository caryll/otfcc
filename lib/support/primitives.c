#include <math.h>
#include "otfcc/primitives.h"

// f2dot14 type
double caryll_from_f2dot14(const f2dot14 x) {
	return x / 16384.0;
}
int16_t caryll_to_f2dot14(const double x) {
	return round(x * 16384.0);
}

// F16.16 (fixed) type
double caryll_from_fixed(const f16dot16 x) {
	return x / 65536.0;
}
f16dot16 caryll_to_fixed(const double x) {
	return round(x * 65536.0);
}
