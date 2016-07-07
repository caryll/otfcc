/*
  Codec of CFF file format and Type2 CharString.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "cff_io.h"

/*
  Number in Type2
  1 32 -246      [-107, 107]           b0 - 139
  2 247-250      [108, 1131]           (b0 - 247) * 256 + b1 + 108
  2 251-254      [-1132, -108]         -(b0 - 251) * 256 - b1 - 108
  3 28 (int16_t) [-32768, 32767]       b1 << 8 | b2
  5 29 (int32_t) [-(2^31), (2^32 - 1)] b1 << 24 | b2 << 16 | b3 << 8 | b4
  * 30 (double)
*/

cff_blob *encode_cff_operator(int32_t val) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	blob->size = (val > 256) ? 2 : 1;
	blob->data = calloc(blob->size, sizeof(uint8_t));

	if (val > 256)
		blob->data[0] = val / 256, blob->data[1] = val % 256;
	else
		blob->data[0] = val;

	return blob;
}

cff_blob *encode_cff_number(int32_t val) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));

	if (val >= -1131 && val <= -108) {
		blob->size = 2;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = (uint8_t)((-108 - val) / 256 + 251);
		blob->data[1] = (uint8_t)((-108 - val) % 256);
	} else if (val >= -107 && val <= 107) {
		blob->size = 1;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = (uint8_t)(val + 139);
	} else if (val >= 108 && val <= 1131) {
		blob->size = 2;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = (uint8_t)((val + 108) / 256 + 247);
		blob->data[1] = (uint8_t)((val + 108) % 256);
	} else if (val >= -32768 && val <= 32767) {
		blob->size = 3;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = 28;
		blob->data[1] = (uint8_t)(val >> 8);
		blob->data[2] = (uint8_t)((val << 8) >> 8);
	} else if (val >= -2147483648 && val <= 2147483647) {
		blob->size = 5;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = 29;
		blob->data[1] = (uint8_t)((val >> 16) / 256);
		blob->data[2] = (uint8_t)((val >> 16) % 256);
		blob->data[3] = (uint8_t)(((val << 16) >> 16) / 256);
		blob->data[4] = (uint8_t)(((val << 16) >> 16) % 256);
	}

	return blob;
}

// -2.25       -> 1e e2 a2 5f
// 0.140541E-3 -> 1e 0a 14 05 41 c3 ff
cff_blob *encode_cff_real(double val) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	uint32_t i, j = 0;
	uint8_t temp[32] = {0};

	if (val == 0.0) {
		blob->size = 2;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = 30;
		blob->data[1] = 0x0f;
	} else {
		uint32_t niblen = 0;
		uint8_t *array;
		sprintf((char *) temp, "%.13g", val);

		for (i = 0; i < strlen((char *) temp);) {
			if (temp[i] == '.')
				niblen++, i++;
			else if (temp[i] >= '0' && temp[i] <= '9')
				niblen++, i++;
			else if (temp[i] == 'e' && temp[i + 1] == '-')
				niblen++, i += 2;
			else if (temp[i] == 'e' && temp[i + 1] == '+')
				niblen++, i += 2;
			else if (temp[i] == '-')
				niblen++, i++;
		}

		blob->size = 2 + niblen / 2;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = 30;

		if (niblen % 2 != 0) {
			array = calloc((niblen + 1), sizeof(uint8_t));
			array[niblen] = 0x0f;
		} else {
			array = calloc((niblen + 2), sizeof(uint8_t));
			array[niblen + 1] = 0x0f;
			array[niblen] = 0x0f;
		}

		for (i = 0; i < strlen((char *) temp);) {
			if (temp[i] == '.')
				array[j++] = 0x0a, i++;
			else if (temp[i] >= '0' && temp[i] <= '9')
				array[j++] = temp[i] - '0', i++;
			else if (temp[i] == 'e' && temp[i + 1] == '-')
				array[j++] = 0x0c, i += 2;
			else if (temp[i] == 'e' && temp[i + 1] == '+')
				array[j++] = 0x0b, i += 2;
			else if (temp[i] == '-')
				array[j++] = 0x0e, i++;
		}

		for (i = 1; i < blob->size; i++) {
			blob->data[i] = array[(i - 1) * 2] * 16 + array[(i - 1) * 2 + 1];
		}

		free(array);
	}

	return blob;
}

// same as encode_cff_operator.
cff_blob *encode_cs2_operator(int32_t val) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));

	if (val > 256)
		blob->size = 2;
	else
		blob->size = 1;

	blob->data = calloc(blob->size, sizeof(uint8_t));

	if (val > 256)
		blob->data[0] = val / 256, blob->data[1] = val % 256;
	else
		blob->data[0] = val;

	return blob;
}

/*
  Number in Type2 CharString
  1 32-246  [-107, 107]   v-139
  2 247-250 [108, 1131]   (v-247) * 256 + w + 108
  2 251-254 [-1131, -108] -(v-251) * 256 - w - 108
*/

cff_blob *encode_cs2_number(int32_t val) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));

	if (val >= -1131 && val <= -108) {
		blob->size = 2;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = (uint8_t)((-108 - val) / 256 + 251);
		blob->data[1] = (uint8_t)((-108 - val) % 256);
	} else if (val >= -107 && val <= 107) {
		blob->size = 1;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = (uint8_t)(val + 139);
	} else if (val >= 108 && val <= 1131) {
		blob->size = 2;
		blob->data = calloc(blob->size, sizeof(uint8_t));
		blob->data[0] = (uint8_t)((val - 108) / 256 + 247);
		blob->data[1] = (uint8_t)((val - 108) % 256);
	} else {
		if (val >= -32768 && val <= 32767) {
			blob->size = 3;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 28;
			blob->data[1] = (uint8_t)(val >> 8);
			blob->data[2] = (uint8_t)((val << 8) >> 8);
		} else
			printf("Error: Ilegal Number (%d) in Type2 CharString.\n", val);
	}

	return blob;
}

cff_blob *encode_cs2_real(double val) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	double val1 = val * 65536.0;

	blob->size = 5;
	blob->data = calloc(5, sizeof(uint8_t));
	blob->data[1] = 255;
	blob->data[2] = ((int32_t)val1 / 65536) / 256;
	blob->data[3] = ((int32_t)val1 / 65536) % 256;
	blob->data[4] = ((int32_t)val1 % 65536) / 256;
	blob->data[5] = ((int32_t)val1 % 65536) % 256;

	return blob;
}

uint32_t decode_cs2_token(uint8_t *start, CFF_Value *val) {
	uint32_t advance = 0;

	if (*start >= 0 && *start <= 27) {
		val->t = CS2_OPERATOR;

		if (*start >= 0 && *start <= 11) {
			val->i = *start;
			advance = 1;
		} else if (*start == 12) {
			val->i = *start << 8 | *(start + 1);
			advance = 2;
		} else if (*start >= 13 && *start <= 18) {
			val->i = *start;
			advance = 1;
		} else if (*start >= 19 && *start <= 20) {
			val->i = *start;
			advance = 1;
		} else if (*start >= 21 && *start <= 27) {
			val->i = *start;
			advance = 1;
		}
	} else if (*start == 28) {
		val->t = CS2_OPERAND;
		val->i = (int16_t)(*(start + 1) << 8 | *(start + 2));
		advance = 3;
	} else if (*start >= 29 && *start <= 31) {
		val->t = CS2_OPERATOR;
		val->i = *start;
		advance = 1;
	} else if (*start >= 32 && *start <= 254) {
		val->t = CS2_OPERAND;

		if (*start >= 32 && *start <= 246) {
			val->i = (int32_t)(*start - 139);
			advance = 1;
		} else if (*start >= 247 && *start <= 250) {
			val->i = (int32_t)((*start - 247) * 256 + *(start + 1) + 108);
			advance = 2;
		} else if (*start >= 251 && *start <= 254) {
			val->i = (int32_t)(-((*start - 251) * 256) - *(start + 1) - 108);
			advance = 2;
		}
	} else if (*start == 255) {
		val->t = CS2_FRACTION;
		val->d = (double)((*(start + 1) << 8 | *(start + 2)) +
		                  (*(start + 3) << 8 | *(start + 4)) / 65536.0);
		advance = 5;
	}

	if (val->t == CS2_OPERAND) val->d = (double)val->i, val->t = CS2_FRACTION;

	return advance;
}

// decode integer
static uint32_t cff_dec_i(uint8_t *start, CFF_Value *val) {
	uint8_t b0 = *start, b1, b2, b3, b4;
	uint32_t len = 0;

	if (b0 >= 32 && b0 <= 246) {
		val->i = b0 - 139;
		len = 1;
	} else if (b0 >= 247 && b0 <= 250) {
		b1 = *(start + 1);
		val->i = (b0 - 247) * 256 + b1 + 108;
		len = 2;
	} else if (b0 >= 251 && b0 <= 254) {
		b1 = *(start + 1);
		val->i = -(b0 - 251) * 256 - b1 - 108;
		len = 2;
	} else if (b0 == 28) {
		b1 = *(start + 1);
		b2 = *(start + 2);
		val->i = (b1 << 8) | b2;
		len = 3;
	} else if (b0 == 29) {
		b1 = *(start + 1);
		b2 = *(start + 2);
		b3 = *(start + 3);
		b4 = *(start + 4);
		val->i = b1 << 24 | b2 << 16 | b3 << 8 | b4;
		len = 5;
	}

	val->t = CFF_INTEGER;
	return len;
}

static const int nibble_attr[15] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 1};
static const char *nibble_symb[15] = {"0", "1", "2", "3", "4",  "5", "6", "7",
                                      "8", "9", ".", "E", "E-", "",  "-"};

// decode double
static uint32_t cff_dec_r(uint8_t *start, CFF_Value *val) {
	uint8_t *nibst, restr[72] = {0};
	size_t str_len = 0;
	uint32_t len;
	uint8_t a, b;

	nibst = start + 1;

	while (1) {
		a = *nibst / 16;
		b = *nibst % 16;

		if (a != 15)
			str_len += nibble_attr[a];
		else
			break;

		if (b != 15)
			str_len += nibble_attr[b];
		else
			break;

		nibst++;
	}

	len = nibst - start + 1;
	nibst = start + 1;

	while (1) {
		a = *nibst / 16;
		b = *nibst % 16;

		if (a != 0x0f)
			strcat((char *) restr, nibble_symb[a]);
		else
			break;

		if (b != 0x0f)
			strcat((char *) restr, nibble_symb[b]);
		else
			break;

		nibst++;
	}

	val->d = atof((char *) restr);
	val->t = CFF_DOUBLE;

	return len;
}

// decode operator
static uint32_t cff_dec_o(uint8_t *start, CFF_Value *val) {
	uint8_t b0 = *start, b1;
	uint32_t len = 0;

	if (b0 >= 0 && b0 <= 21) {
		if (b0 != 12) {
			val->i = b0;
			len = 1;
		} else {
			b1 = *(start + 1);
			val->i = b0 * 256 + b1;
			len = 2;
		}
	}

	val->t = CFF_OPERATOR;

	return len;
}

// error in parsing, return a integer
static uint32_t cff_dec_e(uint8_t *start, CFF_Value *val) {
	printf("Undefined Byte in CFF: %d.\n", *start);
	val->i = *start;
	val->t = CFF_INTEGER;
	return 1;
}

static uint32_t (*_de_t2[256])(uint8_t *, CFF_Value *) = {
    cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o,
    cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o,
    cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_e, cff_dec_e,
    cff_dec_e, cff_dec_e, cff_dec_e, cff_dec_e, cff_dec_i, cff_dec_i, cff_dec_r, cff_dec_e,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_e};

uint32_t decode_cff_token(uint8_t *start, CFF_Value *val) { return _de_t2[*start](start, val); }
