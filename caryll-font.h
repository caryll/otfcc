#ifndef CARYLL_FONT_H

#include "caryll-sfnt.h"

#define foreach(item, array, size)                                                                                     \
	for (int keep = 1, count = 0; keep && count != size; keep = !keep, count++)                                        \
		for (item = (array)[count]; keep; keep = !keep)
#define FOR_TABLE(name, table)                                                                                         \
	foreach (caryll_piece table, packet.pieces, packet.numTables)                                                      \
		if (table.tag == name)

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
};

caryll_font *caryll_font_open(caryll_sfnt *sfnt, uint32_t index);
void caryll_font_close(caryll_font *font);

#endif
