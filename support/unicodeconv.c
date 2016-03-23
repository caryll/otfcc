#include "unicodeconv.h"
sds utf16le_to_utf8(const uint8_t *inb, int inlenb) {
	uint16_t *in = (uint16_t *)inb;
	uint16_t *inend;
	uint32_t c, d, inlen;
	int bits;

	if ((inlenb % 2) == 1) (inlenb)--;
	inlen = inlenb / 2;
	inend = in + inlen;
	// pass 1: calculate bytes used for output
	uint32_t bytesNeeded = 0;
	while (in < inend) {
		c = *in++;
		if ((c & 0xFC00) == 0xD800) { /* surrogates */
			if (in >= inend) {        /* (in > inend) shouldn't happens */
				break;
			}
			d = *in++;
			if ((d & 0xFC00) == 0xDC00) {
				c &= 0x03FF;
				c <<= 10;
				c |= d & 0x03FF;
				c += 0x10000;
			}
		}
		if (c < 0x80) {
			bytesNeeded += 1;
		} else if (c < 0x800) {
			bytesNeeded += 2;
		} else if (c < 0x10000) {
			bytesNeeded += 3;
		} else {
			bytesNeeded += 4;
		}
	}
	in = (uint16_t *)inb;
	const uint8_t *processed = inb;
	sds out = sdsnewlen("", bytesNeeded);
	sds out0 = out;

	while (in < inend) {
		c = *in++;
		if ((c & 0xFC00) == 0xD800) { /* surrogates */
			if (in >= inend) {        /* (in > inend) shouldn't happens */
				break;
			}
			d = *in++;
			if ((d & 0xFC00) == 0xDC00) {
				c &= 0x03FF;
				c <<= 10;
				c |= d & 0x03FF;
				c += 0x10000;
			}
		}

		/* assertion: c is a single UTF-4 value */
		if (c < 0x80) {
			*out++ = c;
			bits = -6;
		} else if (c < 0x800) {
			*out++ = ((c >> 6) & 0x1F) | 0xC0;
			bits = 0;
		} else if (c < 0x10000) {
			*out++ = ((c >> 12) & 0x0F) | 0xE0;
			bits = 6;
		} else {
			*out++ = ((c >> 18) & 0x07) | 0xF0;
			bits = 12;
		}

		for (; bits >= 0; bits -= 6) {
			*out++ = ((c >> bits) & 0x3F) | 0x80;
		}
		processed = (const uint8_t *)in;
	}
	return out0;
}

sds utf16be_to_utf8(const uint8_t *inb, int inlenb) {
	uint16_t *in = (uint16_t *)inb;
	uint16_t *inend;
	uint32_t c, d, inlen;
	uint8_t *tmp;
	int bits;

	if ((inlenb % 2) == 1) (inlenb)--;
	inlen = inlenb / 2;
	inend = in + inlen;
	// pass 1: calculate bytes used for output
	uint32_t bytesNeeded = 0;
	while (in < inend) {
		{
			tmp = (uint8_t *)in;
			c = *tmp++;
			c = c << 8;
			c = c | (uint32_t)*tmp;
			in++;
		}
		if ((c & 0xFC00) == 0xD800) { /* surrogates */
			if (in >= inend) {        /* (in > inend) shouldn't happens */
				break;
			}
			{
				tmp = (uint8_t *)in;
				d = *tmp++;
				d = d << 8;
				d = d | (uint32_t)*tmp;
				in++;
			}
			if ((d & 0xFC00) == 0xDC00) {
				c &= 0x03FF;
				c <<= 10;
				c |= d & 0x03FF;
				c += 0x10000;
			}
		}
		if (c < 0x80) {
			bytesNeeded += 1;
		} else if (c < 0x800) {
			bytesNeeded += 2;
		} else if (c < 0x10000) {
			bytesNeeded += 3;
		} else {
			bytesNeeded += 4;
		}
	}
	in = (uint16_t *)inb;
	const uint8_t *processed = inb;
	sds out = sdsnewlen("", bytesNeeded);
	sds out0 = out;

	while (in < inend) {
		{
			tmp = (uint8_t *)in;
			c = *tmp++;
			c = c << 8;
			c = c | (uint32_t)*tmp;
			in++;
		}
		if ((c & 0xFC00) == 0xD800) { /* surrogates */
			if (in >= inend) {        /* (in > inend) shouldn't happens */
				break;
			}
			{
				tmp = (uint8_t *)in;
				d = *tmp++;
				d = d << 8;
				d = d | (uint32_t)*tmp;
				in++;
			}
			if ((d & 0xFC00) == 0xDC00) {
				c &= 0x03FF;
				c <<= 10;
				c |= d & 0x03FF;
				c += 0x10000;
			}
		}

		/* assertion: c is a single UTF-4 value */
		if (c < 0x80) {
			*out++ = c;
			bits = -6;
		} else if (c < 0x800) {
			*out++ = ((c >> 6) & 0x1F) | 0xC0;
			bits = 0;
		} else if (c < 0x10000) {
			*out++ = ((c >> 12) & 0x0F) | 0xE0;
			bits = 6;
		} else {
			*out++ = ((c >> 18) & 0x07) | 0xF0;
			bits = 12;
		}

		for (; bits >= 0; bits -= 6) {
			*out++ = ((c >> bits) & 0x3F) | 0x80;
		}
		processed = (const uint8_t *)in;
	}
	return out0;
}
