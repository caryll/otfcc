#ifndef CARYLL_SUPPORT_BUFFER_H
#define CARYLL_SUPPORT_BUFFER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../../extern/sds.h"
typedef struct {
	size_t cursor;
	sds s;
} caryll_buffer;

caryll_buffer *bufnew();
void buffree(caryll_buffer *buf);
size_t buflen(caryll_buffer *buf);
size_t bufpos(caryll_buffer *buf);
void bufseek(caryll_buffer *buf, size_t pos);
void bufclear(caryll_buffer *buf);

void bufwrite8(caryll_buffer *buf, uint8_t byte);
void bufwrite16l(caryll_buffer *buf, uint16_t x);
void bufwrite16b(caryll_buffer *buf, uint16_t x);
void bufwrite32l(caryll_buffer *buf, uint32_t x);
void bufwrite32b(caryll_buffer *buf, uint32_t x);
void bufwrite64l(caryll_buffer *buf, uint64_t x);
void bufwrite64b(caryll_buffer *buf, uint64_t x);

void bufwrite_sds(caryll_buffer *buf, sds str);
void bufwrite_str(caryll_buffer *buf, const char *str);
void bufwrite_bytes(caryll_buffer *buf, size_t size, uint8_t *str);
void bufwrite_buf(caryll_buffer *buf, caryll_buffer *that);
void bufwrite_bufdel(caryll_buffer *buf, caryll_buffer *that);

void bufping16b(caryll_buffer *buf, size_t *offset, size_t *cp);
void bufping16bd(caryll_buffer *buf, size_t *offset, size_t *shift, size_t *cp);
void bufpingpong16b(caryll_buffer *buf, caryll_buffer *that, size_t *offset, size_t *cp);
void bufpong(caryll_buffer *buf, size_t *offset, size_t *cp);

void buflongalign(caryll_buffer *buf);

#endif
