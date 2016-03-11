#pragma once

#include <stdio.h>
#include <stdint.h>

uint16_t caryll_blt16u(uint8_t *src);
uint32_t caryll_blt32u(uint8_t *src);
uint64_t caryll_blt64u(uint8_t *src);
uint16_t caryll_get16u(FILE *file);
uint32_t caryll_get32u(FILE *file);
uint64_t caryll_get64u(FILE *file);
