#include "buffer.h"

caryll_buffer *bufnew() {
	caryll_buffer *buf = malloc(sizeof(caryll_buffer));
	buf->size = 0;
	buf->free = 0;
	buf->cursor = 0;
	buf->data = NULL;
	return buf;
}
void buffree(caryll_buffer *buf) {
	if (!buf) return;
	if (buf->data) free(buf->data);
	free(buf);
}
size_t buflen(caryll_buffer *buf) {
	return buf->size;
}
size_t bufpos(caryll_buffer *buf) {
	return buf->cursor;
}
void bufseek(caryll_buffer *buf, size_t pos) {
	buf->cursor = pos;
}
void bufclear(caryll_buffer *buf) {
	buf->cursor = 0;
	buf->free = buf->size + buf->free;
	buf->size = 0;
}

static void bufbeforewrite(caryll_buffer *buf, size_t towrite) {
	size_t currentSize = buf->size;
	size_t allocated = buf->size + buf->free;
	size_t required = buf->cursor + towrite;
	if (required < currentSize) {
		// Completely overlap.
		return;
	} else if (required <= allocated) {
		// Within range without reallocation
		buf->size = required;
		buf->free = allocated - buf->size;
	} else {
		// Needs realloc
		buf->size = required;
		buf->free = required; // Double growth
		if (buf->free > 0x1000000) { buf->free = 0x1000000; }
		if (buf->data) {
			buf->data = realloc(buf->data, sizeof(uint8_t) * (buf->size + buf->free));
		} else {
			buf->data = calloc(buf->size + buf->free, sizeof(uint8_t));
		}
	}
}
void bufwrite8(caryll_buffer *buf, uint8_t byte) {
	bufbeforewrite(buf, 1);
	buf->data[buf->cursor++] = byte;
}
void bufwrite16l(caryll_buffer *buf, uint16_t x) {
	bufbeforewrite(buf, 2);
	buf->data[buf->cursor++] = x & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
}
void bufwrite16b(caryll_buffer *buf, uint16_t x) {
	bufbeforewrite(buf, 2);
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = x & 0xFF;
}
void bufwrite32l(caryll_buffer *buf, uint32_t x) {
	bufbeforewrite(buf, 4);
	buf->data[buf->cursor++] = x & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
	buf->data[buf->cursor++] = (x >> 24) & 0xFF;
}
void bufwrite32b(caryll_buffer *buf, uint32_t x) {
	bufbeforewrite(buf, 4);
	buf->data[buf->cursor++] = (x >> 24) & 0xFF;
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = x & 0xFF;
}
void bufwrite64l(caryll_buffer *buf, uint64_t x) {
	bufbeforewrite(buf, 8);
	buf->data[buf->cursor++] = x & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
	buf->data[buf->cursor++] = (x >> 24) & 0xFF;
	buf->data[buf->cursor++] = (x >> 32) & 0xFF;
	buf->data[buf->cursor++] = (x >> 40) & 0xFF;
	buf->data[buf->cursor++] = (x >> 48) & 0xFF;
	buf->data[buf->cursor++] = (x >> 56) & 0xFF;
}
void bufwrite64b(caryll_buffer *buf, uint64_t x) {
	bufbeforewrite(buf, 8);
	buf->data[buf->cursor++] = (x >> 56) & 0xFF;
	buf->data[buf->cursor++] = (x >> 48) & 0xFF;
	buf->data[buf->cursor++] = (x >> 40) & 0xFF;
	buf->data[buf->cursor++] = (x >> 32) & 0xFF;
	buf->data[buf->cursor++] = (x >> 24) & 0xFF;
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = x & 0xFF;
}

caryll_buffer *bufninit(uint32_t n, ...) {
	caryll_buffer *buf = bufnew();
	bufbeforewrite(buf, n);
	va_list ap;
	va_start(ap, n);
	for (uint16_t j = 0; j < n; j++) {
		bufwrite8(buf, (uint8_t)va_arg(ap, int));
	}
	va_end(ap);
	return buf;
}
void bufnwrite8(caryll_buffer *buf, uint32_t n, ...) {
	bufbeforewrite(buf, n);
	va_list ap;
	va_start(ap, n);
	for (uint16_t j = 0; j < n; j++) {
		bufwrite8(buf, (uint8_t)va_arg(ap, int));
	}
	va_end(ap);
}

void bufwrite_sds(caryll_buffer *buf, sds str) {
	if (!str) return;
	size_t len = sdslen(str);
	if (!len) return;
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, str, len);
	buf->cursor += len;
}
void bufwrite_str(caryll_buffer *buf, const char *str) {
	if (!str) return;
	size_t len = strlen(str);
	if (!len) return;
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, str, len);
	buf->cursor += len;
}
void bufwrite_bytes(caryll_buffer *buf, size_t len, uint8_t *str) {
	if (!str) return;
	if (!len) return;
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, str, len);
	buf->cursor += len;
}
void bufwrite_buf(caryll_buffer *buf, caryll_buffer *that) {
	if (!that || !that->data) return;
	size_t len = buflen(that);
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, that->data, len);
	buf->cursor += len;
}
void bufwrite_bufdel(caryll_buffer *buf, caryll_buffer *that) {
	if (!that || !that->data) return;
	size_t len = buflen(that);
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, that->data, len);
	buffree(that);
	buf->cursor += len;
}

void buflongalign(caryll_buffer *buf) {
	size_t cp = buf->cursor;
	bufseek(buf, buflen(buf));
	if (buflen(buf) % 4 == 1) {
		bufwrite8(buf, 0);
		bufwrite8(buf, 0);
		bufwrite8(buf, 0);
	} else if (buflen(buf) % 4 == 2) {
		bufwrite8(buf, 0);
		bufwrite8(buf, 0);
	} else if (buflen(buf) % 4 == 3) {
		bufwrite8(buf, 0);
	}
	bufseek(buf, cp);
}

// bufpingpong16b writes a buffer and an offset towards it.
// [ ^                            ] + ###### that
//   ^cp             ^offset
//                           |
//                           V
// [ @^              ######       ] , and the value of [@] equals to the former
// offset.
//    ^cp                  ^offset
// Common in writing OpenType features.
void bufping16b(caryll_buffer *buf, size_t *offset, size_t *cp) {
	bufwrite16b(buf, *offset);
	*cp = buf->cursor;
	bufseek(buf, *offset);
}
void bufping16bd(caryll_buffer *buf, size_t *offset, size_t *shift, size_t *cp) {
	bufwrite16b(buf, *offset - *shift);
	*cp = buf->cursor;
	bufseek(buf, *offset);
}
void bufpong(caryll_buffer *buf, size_t *offset, size_t *cp) {
	*offset = buf->cursor;
	bufseek(buf, *cp);
}
void bufpingpong16b(caryll_buffer *buf, caryll_buffer *that, size_t *offset, size_t *cp) {
	bufwrite16b(buf, *offset);
	*cp = buf->cursor;
	bufseek(buf, *offset);
	bufwrite_bufdel(buf, that);
	*offset = buf->cursor;
	bufseek(buf, *cp);
}
