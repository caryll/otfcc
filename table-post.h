#pragma once

#include <stdint.h>

typedef struct {
  // PostScript information
  uint32_t version;
  uint32_t italicAngle;
  int16_t  underlinePosition;
  int16_t  underlineThickness;
  uint32_t isFixedPitch;
  uint32_t minMemType42;
  uint32_t maxMemType42;
  uint32_t minMemType1;
  uint32_t maxMemType1;
} table_post;