#ifndef CARYLL_FONT_H
#define CARYLL_FONT_H
#include "caryll-sfnt.h"
#include "support/util.h"

struct _caryll_font;
typedef struct _caryll_font caryll_Font;

#include "support/glyph-order.h"

#include "tables/OS_2.h"
#include "tables/cmap.h"
#include "tables/cvt.h"
#include "tables/fpgm-prep.h"
#include "tables/gasp.h"
#include "tables/glyf.h"
#include "tables/head.h"
#include "tables/hhea.h"
#include "tables/hmtx.h"
#include "tables/maxp.h"
#include "tables/name.h"
#include "tables/post.h"

#include "tables/CFF.h"

#include "tables/LTSH.h"
#include "tables/hdmx.h"
#include "tables/vhea.h"
#include "tables/vmtx.h"
#include "tables/VORG.h"

#include <tables/otl/GDEF.h>
#include <tables/otl/BASE.h>
#include <tables/otl/otl.h>

typedef enum { FONTTYPE_TTF, FONTTYPE_CFF } caryll_font_subtype;

struct _caryll_font {
	caryll_font_subtype subtype;

	table_head *head;
	table_hhea *hhea;
	table_maxp *maxp;
	table_OS_2 *OS_2;
	table_hmtx *hmtx;
	table_post *post;
	table_hdmx *hdmx;

	table_vhea *vhea;
	table_vmtx *vmtx;
	table_VORG *VORG;

	table_CFF *CFF_;
	table_glyf *glyf;
	table_cmap *cmap;
	table_name *name;

	table_fpgm_prep *fpgm;
	table_fpgm_prep *prep;
	table_cvt *cvt_;
	table_gasp *gasp;

	table_LTSH *LTSH;

	table_OTL *GSUB;
	table_OTL *GPOS;
	table_GDEF *GDEF;
	table_BASE *BASE;

	caryll_GlyphOrder *glyph_order;
};

caryll_font_subtype caryll_decideFontSubtype(caryll_SplineFontContainer *sfnt, uint32_t index);
caryll_font_subtype caryll_decideFontSubtypeFromJson(json_value *root);
caryll_Font *caryll_new_Font();
void caryll_delete_Font(caryll_Font *font);
caryll_Font *caryll_read_Font(caryll_SplineFontContainer *sfnt, uint32_t index, caryll_Options *options);
json_value *caryll_dump_Font(caryll_Font *font, caryll_Options *options);
caryll_Font *caryll_parse_Font(json_value *root, caryll_Options *options);
caryll_buffer *caryll_build_Font(caryll_Font *font, caryll_Options *options);

#endif
