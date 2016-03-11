#pragma once

#include "table-head.h"
#include "table-hhea.h"
#include "table-maxp.h"
#include "table-hmtx.h"
#include "table-post.h"

#include "table-hdmx.h"
#include "table-LTSH.h"
#include "table-PCLT.h"
#include "table-vhea.h"
#include "table-vmtx.h"

typedef struct {
	table_head *head;
	table_hhea *hhea;
	table_maxp *maxp;
	table_hmtx *hmtx;
	table_post *post;
	table_hdmx *hdmx;
	table_LTSH *LTSH;
	table_PCLT *PCLT;
	table_vhea *vhea;
	table_vmtx *vmtx;
} caryll_font;

caryll_font *caryll_font_open(caryll_sfnt *sfnt, uint32_t index);
void caryll_font_close(caryll_font *font);
