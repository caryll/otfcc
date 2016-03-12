#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static _Bool caryll_check_endian(void) {
	union {
		uint8_t i1[2];
		uint16_t i2;
	} check_union = {1};

	return (check_union.i1[0] == 1);
}

uint16_t caryll_endian_convert16(uint16_t i) {
	if (caryll_check_endian()) {
		union {
			uint8_t i1[2];
			uint16_t i2;
		} src, des;

		src.i2 = i;

		des.i1[0] = src.i1[1];
		des.i1[1] = src.i1[0];

		return des.i2;
	} else {
		return i;
	}
}

uint32_t caryll_endian_convert32(uint32_t i) {
	if (caryll_check_endian()) {
		union {
			uint8_t i1[4];
			uint32_t i4;
		} src, des;

		src.i4 = i;

		des.i1[0] = src.i1[3];
		des.i1[1] = src.i1[2];
		des.i1[2] = src.i1[1];
		des.i1[3] = src.i1[0];

		return des.i4;
	} else {
		return i;
	}
}

uint64_t caryll_endian_convert64(uint64_t i) {
	if (caryll_check_endian()) {
		union {
			uint8_t i1[8];
			uint64_t i8;
		} src, des;

		src.i8 = i;

		des.i1[0] = src.i1[7];
		des.i1[1] = src.i1[6];
		des.i1[2] = src.i1[5];
		des.i1[3] = src.i1[4];
		des.i1[4] = src.i1[3];
		des.i1[5] = src.i1[2];
		des.i1[6] = src.i1[1];
		des.i1[7] = src.i1[0];

		return des.i8;
	} else {
		return i;
	}
}

uint16_t caryll_get16u(FILE *file) {
	uint16_t tmp;
	fread(&tmp, 2, 1, file);
	return caryll_endian_convert16(tmp);
}

uint32_t caryll_get32u(FILE *file) {
	uint32_t tmp;
	fread(&tmp, 4, 1, file);
	return caryll_endian_convert32(tmp);
}

uint64_t caryll_get64u(FILE *file) {
	uint64_t tmp;
	fread(&tmp, 8, 1, file);
	return caryll_endian_convert64(tmp);
}

uint16_t caryll_blt16u(uint8_t *src) {
	uint16_t b0 = ((uint16_t)*src) << 8;
	uint16_t b1 = ((uint16_t) * (src + 1));

	return (b0 | b1);
}

uint32_t caryll_blt32u(uint8_t *src) {
	uint32_t b0 = ((uint32_t)*src) << 24;
	uint32_t b1 = ((uint32_t) * (src + 1)) << 16;
	uint32_t b2 = ((uint32_t) * (src + 2)) << 8;
	uint32_t b3 = ((uint32_t) * (src + 3));

	return (b0 | b1 | b2 | b3);
}

uint64_t caryll_blt64u(uint8_t *src) {
	uint64_t b0 = ((uint64_t)*src) << 56;
	uint64_t b1 = ((uint64_t) * (src + 1)) << 48;
	uint64_t b2 = ((uint64_t) * (src + 2)) << 40;
	uint64_t b3 = ((uint64_t) * (src + 3)) << 32;
	uint64_t b4 = ((uint64_t) * (src + 4)) << 24;
	uint64_t b5 = ((uint64_t) * (src + 5)) << 16;
	uint64_t b6 = ((uint64_t) * (src + 6)) << 8;
	uint64_t b7 = ((uint64_t) * (src + 7));

	return (b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7);
}
