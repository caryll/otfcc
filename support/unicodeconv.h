#ifndef CARYLL_SUPPORT_UNICODECONV_H
#define CARYLL_SUPPORT_UNICODECONV_H
#include <stdio.h>
#include <stdint.h>
#include "../extern/sds.h"
sds utf16le_to_utf8(const uint8_t *inb, int inlenb);
sds utf16be_to_utf8(const uint8_t *inb, int inlenb);

#endif
