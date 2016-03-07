#pragma once

#include "table-head.h"
#include "table-hhea.h"
#include "table-maxp.h"
#include "table-hmtx.h"
#include "table-post.h"

#include "table-hdmx.h"

typedef struct {
  table_head * head;
  table_hhea * hhea;
  table_maxp * maxp;
  table_hmtx * hmtx;
  table_post * post;
  table_hdmx * hdmx;
} caryll_font;

caryll_font * caryll_font_open(caryll_sfnt * sfnt, uint32_t index);
void caryll_font_close(caryll_font * font);