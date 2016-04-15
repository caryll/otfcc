#ifndef CARYLL_FONT_H
#define CARYLL_FONT_H
#include "caryll-sfnt.h"
#include "../src-support/util.h"

struct _caryll_font;
typedef struct _caryll_font caryll_font;

#include "../src-support/glyphorder.h"

#include "./tables/head.h"
#include "./tables/hhea.h"
#include "./tables/maxp.h"
#include "./tables/hmtx.h"
#include "./tables/post.h"
#include "./tables/OS_2.h"
#include "./tables/name.h"
#include "./tables/glyf.h"
#include "./tables/cmap.h"
#include "./tables/fpgm-prep.h"
#include "./tables/gasp.h"

#include "./tables/hdmx.h"
#include "./tables/LTSH.h"
#include "./tables/PCLT.h"
#include "./tables/vhea.h"
#include "./tables/vmtx.h"

#include "./tables/otl/otl.h"
#include "./tables/otl/GDEF.h"

struct _caryll_font {
	table_head *head;
	table_hhea *hhea;
	table_maxp *maxp;
	table_OS_2 *OS_2;
	table_hmtx *hmtx;
	table_post *post;
	table_hdmx *hdmx;

	table_vhea *vhea;
	table_vmtx *vmtx;

	table_glyf *glyf;
	cmap_hash *cmap;
	table_name *name;

	table_fpgm_prep *fpgm;
	table_fpgm_prep *prep;
	table_fpgm_prep *cvt_;
	table_gasp *gasp;

	table_otl *GSUB;
	table_otl *GPOS;
	table_GDEF *GDEF;

	glyph_order_hash *glyph_order;
};

caryll_font *caryll_new_font();
caryll_font *caryll_read_font(caryll_sfnt *sfnt, uint32_t index);
void caryll_delete_font(caryll_font *font);
json_value *caryll_font_to_json(caryll_font *font, caryll_dump_options *dumpopts);
caryll_font *caryll_font_from_json(json_value *root, caryll_dump_options *dumpopts);
caryll_buffer *caryll_write_font(caryll_font *font, caryll_dump_options *dumpopts);

#endif
