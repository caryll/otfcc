#ifndef CARYLL_FONT_H
#define CARYLL_FONT_H
#include "caryll-sfnt.h"
#include "support/util.h"

struct _caryll_font;
typedef struct _caryll_font caryll_font;

#include "./support/glyphorder.h"

#include "./tables/head.h"
#include "./tables/hhea.h"
#include "./tables/maxp.h"
#include "./tables/hmtx.h"
#include "./tables/post.h"
#include "./tables/OS_2.h"
#include "./tables/name.h"
#include "./tables/glyf.h"
#include "./tables/cmap.h"

#include "./tables/hdmx.h"
#include "./tables/LTSH.h"
#include "./tables/PCLT.h"
#include "./tables/vhea.h"
#include "./tables/vmtx.h"

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
	table_name *name;

	glyph_order_hash *glyph_order;
};

caryll_font *caryll_font_new();
caryll_font *caryll_font_open(caryll_sfnt *sfnt, uint32_t index);
void caryll_font_unconsolidate(caryll_font *font);
void caryll_font_close(caryll_font *font);

#endif
