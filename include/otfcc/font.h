#ifndef CARYLL_FONT_H
#define CARYLL_FONT_H

#include "sfnt.h"

struct _caryll_font;
typedef struct _caryll_font otfcc_Font;

#include "otfcc/glyph-order.h"

#include "otfcc/table/head.h"
#include "otfcc/table/glyf.h"
#include "otfcc/table/CFF.h"
#include "otfcc/table/maxp.h"
#include "otfcc/table/hhea.h"
#include "otfcc/table/hmtx.h"
#include "otfcc/table/hdmx.h"
#include "otfcc/table/vhea.h"
#include "otfcc/table/vmtx.h"
#include "otfcc/table/OS_2.h"
#include "otfcc/table/post.h"
#include "otfcc/table/name.h"

#include "otfcc/table/cmap.h"
#include "otfcc/table/cvt.h"
#include "otfcc/table/fpgm-prep.h"
#include "otfcc/table/gasp.h"

#include "otfcc/table/LTSH.h"
#include "otfcc/table/VORG.h"

#include "otfcc/table/GDEF.h"
#include "otfcc/table/BASE.h"
#include "otfcc/table/otl.h"

typedef enum { FONTTYPE_TTF, FONTTYPE_CFF } otfcc_font_subtype;

struct _caryll_font {
	otfcc_font_subtype subtype;

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

	otfcc_GlyphOrder *glyph_order;
};

otfcc_Font *otfcc_newFont();
void otfcc_deleteFont(otfcc_Font *font);
void otfcc_consolidateFont(otfcc_Font *font, const otfcc_Options *options);

// Font builder interfaces
typedef struct otfcc_IFontBuilder {
	otfcc_Font *(*create)(void *source, uint32_t index, const otfcc_Options *options);
	void (*dispose)(struct otfcc_IFontBuilder *self);
} otfcc_IFontBuilder;
otfcc_IFontBuilder *otfcc_newOTFReader();
otfcc_IFontBuilder *otfcc_newJsonReader();

// Font serializer interface
typedef struct otfcc_IFontSerializer {
	void *(*serialize)(otfcc_Font *font, const otfcc_Options *options);
	void (*dispose)(struct otfcc_IFontSerializer *self);
} otfcc_IFontSerializer;
otfcc_IFontSerializer *otfcc_newJsonWriter();
otfcc_IFontSerializer *otfcc_newOTFWriter();

#endif
