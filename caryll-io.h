#ifndef CARYLL_IO_H
#define CARYLL_IO_H

#include <stdio.h>
#include <stdint.h>

typedef uint8_t *font_file_pointer;

uint16_t caryll_blt16u(uint8_t *src);
uint32_t caryll_blt32u(uint8_t *src);
uint64_t caryll_blt64u(uint8_t *src);
uint16_t caryll_get16u(FILE *file);
uint32_t caryll_get32u(FILE *file);
uint64_t caryll_get64u(FILE *file);

float caryll_from_f2dot14(int16_t x);

#endif
