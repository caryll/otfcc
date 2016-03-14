#ifndef CARYLL_FONT_H

#include "caryll-sfnt.h"

#define FOR_TABLE(name, table)                                                                                         \
	for (int keep = 1, count = 0, __notfound = 1; __notfound && keep && count < packet.numTables;                      \
	     keep = !keep, count++)                                                                                        \
		for (caryll_piece table = (packet.pieces)[count]; keep; keep = !keep)                                          \
			if (table.tag == (name))                                                                                   \
				for (int k2 = 1; k2; k2 = 0, __notfound = 0)

#define CARYLL_FONT_H

struct _caryll_font;
typedef struct _caryll_font caryll_font;

#include "./tables/head.h"
#include "./tables/hhea.h"
#include "./tables/maxp.h"
#include "./tables/hmtx.h"
#include "./tables/post.h"
#include "./tables/OS_2.h"

#include "./tables/hdmx.h"
#include "./tables/LTSH.h"
#include "./tables/PCLT.h"
#include "./tables/vhea.h"
#include "./tables/vmtx.h"

#include "./tables/glyf.h"
#include "./tables/cmap.h"

#include "./support/glyphorder.h"

struct _caryll_font {
	table_head *head;
	table_hhea *hhea;
	table_maxp *maxp;
	table_OS_2 *OS_2;
	table_hmtx *hmtx;
	table_post *post;
	table_hdmx *hdmx;
	table_LTSH *LTSH;
	table_PCLT *PCLT;
	table_vhea *vhea;
	table_vmtx *vmtx;
	table_glyf *glyf;
	cmap_hash *cmap;
	
	glyph_order_hash *glyph_order;
};

caryll_font *caryll_font_open(caryll_sfnt *sfnt, uint32_t index);
void caryll_font_close(caryll_font *font);

#endif
