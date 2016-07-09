#ifndef CFF_DATA_TYPES
#define CFF_DATA_TYPES

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/util.h>

// clang-format off
// CFF DICT Operators
enum {
	op_version          = 0x00, op_Copyright          = 0x0c00,
	op_Notice           = 0x01, op_isFixedPitch       = 0x0c01,
	op_FullName         = 0x02, op_ItalicAngle        = 0x0c02,
	op_FamilyName       = 0x03, op_UnderlinePosition  = 0x0c03,
	op_Weight           = 0x04, op_UnderlineThickness = 0x0c04,
	op_FontBBox         = 0x05, op_PaintType          = 0x0c05,
	op_BlueValues       = 0x06, op_CharstringType     = 0x0c06,
	op_OtherBlues       = 0x07, op_FontMatrix         = 0x0c07,
	op_FamilyBlues      = 0x08, op_StrokeWidth        = 0x0c08,
	op_FamilyOtherBlues = 0x09, op_BlueScale          = 0x0c09,
	op_StdHW            = 0x0a, op_BlueShift          = 0x0c0a,
	op_StdVW            = 0x0b, op_BlueFuzz           = 0x0c0b,
	/* 0x0c escape */           op_StemSnapH          = 0x0c0c,
	op_UniqueID         = 0x0d, op_StemSnapV          = 0x0c0d,
	op_XUID             = 0x0e, op_ForceBold          = 0x0c0e,
	op_charset          = 0x0f, /* 0x0c0f Reserved */
	op_Encoding         = 0x10, /* 0x0c10 Reserved */
	op_CharStrings      = 0x11, op_LanguageGroup      = 0x0c11,
	op_Private          = 0x12, op_ExpansionFactor    = 0x0c12,
	op_Subrs            = 0x13, op_initialRandomSeed  = 0x0c13,
	op_defaultWidthX    = 0x14, op_SyntheicBase       = 0x0c14,
	op_nominalWidthX    = 0x15, op_PostScript         = 0x0c15,
								op_BaseFontName       = 0x0c16,
								op_BaseFontBlend      = 0x0c17,
								/* 0x0c18 Reserved */
								/* 0x0c19 Reserved */
								/* 0x0c1a Reserved */
								/* 0x0c1b Reserved */
								/* 0x0c1c Reserved */
								/* 0x0c1d Reserved */
								op_ROS                = 0x0c1e,
								op_CIDFontVersion     = 0x0c1f,
								op_CIDFontRevision    = 0x0c20,
								op_CIDFontType        = 0x0c21,
								op_CIDCount           = 0x0c22,
								op_UIDBase            = 0x0c23,
								op_FDArray            = 0x0c24,
								op_FDSelect           = 0x0c25,
								op_FontName           = 0x0c26,
};

// Type2 CharString Operators
enum {
	/* 0x00 Reserved */   /* 0x0c00 Reserved */
	op_hstem      = 0x01, /* 0x0c01 Reserved */
	/* 0x02 Reserved */   /* 0x0c02 Reserved */
	op_vstem      = 0x03, op_and    = 0x0c03,
	op_vmoveto    = 0x04, op_or     = 0x0c04,
	op_rlineto    = 0x05, op_not    = 0x0c05,
	op_hlineto    = 0x06, /* 0x0c06 Reserved */
	op_vlineto    = 0x07, /* 0x0c07 Reserved */
	op_rrcurveto  = 0x08, /* 0x0c08 Reserved */
	/* 0x09 Reserved */   op_abs    = 0x0c09,
	op_callsubr   = 0x0a, op_add    = 0x0c0a,
	op_return     = 0x0b, op_sub    = 0x0c0b,
	/* 0x0c escape   */   op_div    = 0x0c0c,
	/* 0x0d Reserved */   /* 0x0c0d Reserved */
	op_endchar    = 0x0e, op_neg    = 0x0c0e,
	/* 0x0f Reserved */   op_eq     = 0x0c0f,
	/* 0x10 Reserved */   /* 0x0c10 Reserved */
	/* 0x11 Reserved */   /* 0x0c11 Reserved */
	op_hstemhm    = 0x12, op_drop   = 0x0c12,
	op_hintmask   = 0x13, /* 0x0c13 Reserved */
	op_cntrmask   = 0x14, op_put    = 0x0c14,
	op_rmoveto    = 0x15, op_get    = 0x0c15,
	op_hmoveto    = 0x16, op_ifelse = 0x0c16,
	op_vstemhm    = 0x17, op_random = 0x0c17,
	op_rcurveline = 0x18, op_mul    = 0x0c18,
	op_rlinecurve = 0x19, /* 0x0c19 Reserved */
	op_vvcurveto  = 0x1a, op_sqrt   = 0x0c1a,
	op_hhcurveto  = 0x1b, op_dup    = 0x0c1b,
	/* 0x1c short int */  op_exch   = 0x0c1c,
	op_callgsubr  = 0x1d, op_index  = 0x0c1d,
	op_vhcurveto  = 0x1e, op_roll   = 0x0c1e,
	op_hvcurveto  = 0x1f, /* 0x0c1f Reserved */
						/* 0x0c20 Reserved */
						/* 0x0c21 Reserved */
						op_hflex  = 0x0c22,
						op_flex   = 0x0c23,
						op_hflex1 = 0x0c24,
						op_flex1  = 0x0c25,
};
// clang-format on

// Limits of Type2 CharSrting
enum {
	type2_argument_stack = 48,
	type2_stem_hints = 96,
	type2_subr_nesting = 10,
	type2_charstring_len = 65535,
	type2_max_subrs = 65536,
	type2_transient_array = 32,
};

typedef struct {
	uint16_t count;
	uint8_t offSize;
	uint32_t *offset;
	uint8_t *data;
} CFF_INDEX;

typedef struct {
	uint8_t major;
	uint8_t minor;
	uint8_t hdrSize;
	uint8_t offSize;
} CFF_Header;

// CFF Encoding Structures

typedef struct {
	uint8_t format;
	uint8_t ncodes;
	uint8_t *code;
} encoding_f0;

typedef struct {
	uint8_t first;
	uint8_t nleft;
} enc_range1;

typedef struct {
	uint8_t format;
	uint8_t nranges;
	enc_range1 *range1;
} encoding_f1;

typedef struct {
	uint8_t code;
	uint16_t glyph;
} enc_supplement;

typedef struct {
	uint8_t nsup;
	enc_supplement *supplement;
} encoding_ns;

typedef struct {
	uint32_t t;
	union {
		encoding_f0 f0;
		encoding_f1 f1;
		encoding_ns ns;
	};
} CFF_Encoding;

// CFF Charset Structures

typedef struct {
	uint8_t format;
	uint16_t *glyph;
} charset_f0;

typedef struct {
	uint16_t first;
	uint8_t nleft;
} charset_range1;

typedef struct {
	uint8_t format;
	charset_range1 *range1;
} charset_f1;

typedef struct {
	uint16_t first;
	uint16_t nleft;
} charset_range2;

typedef struct {
	uint8_t format;
	charset_range2 *range2;
} charset_f2;

typedef struct {
	uint32_t t;
	uint32_t s; // size
	union {
		charset_f0 f0;
		charset_f1 f1;
		charset_f2 f2;
	};
} CFF_Charset;

// CFF FDSelect Structures

typedef struct {
	uint8_t format;
	uint8_t *fds;
} fdselect_f0;

typedef struct {
	uint16_t first;
	uint8_t fd;
} fdselect_range3;

typedef struct {
	uint8_t format;
	uint16_t nranges;
	fdselect_range3 *range3;
	uint16_t sentinel;
} fdselect_f3;

typedef struct {
	uint32_t t;
	uint32_t s;
	union {
		fdselect_f0 f0;
		fdselect_f3 f3;
	};
} CFF_FDSelect;

// Predefined Encoding Types

enum {
	CFF_FONT_COMMON,
	CFF_FONT_CID,
	CFF_FONT_MM,
};

enum {
	CFF_ENC_STANDARD,
	CFF_ENC_EXPERT,
	CFF_ENC_FORMAT0,
	CFF_ENC_FORMAT1,
	CFF_ENC_FORMAT_SUPPLEMENT,
	CFF_ENC_UNSPECED,
};

enum {
	CFF_CHARSET_ISOADOBE = 0,
	CFF_CHARSET_UNSPECED = 0,
	CFF_CHARSET_EXPERT = 1,
	CFF_CHARSET_EXPERTSUBSET = 2,
	CFF_CHARSET_FORMAT0 = 3,
	CFF_CHARSET_FORMAT1 = 4,
	CFF_CHARSET_FORMAT2 = 5,
};

enum {
	CFF_FDSELECT_FORMAT0,
	CFF_FDSELECT_FORMAT3,
	CFF_FDSELECT_UNSPECED,
};

enum {
	CS2_OPERATOR,
	CS2_OPERAND,
	CS2_FRACTION,
};

enum {
	CFF_OPERATOR,
	CFF_INTEGER,
	CFF_DOUBLE,
};

enum { CFF_LIMIT_STACK = 48, CFF_LIMIT_TRANSIENT = 32 };

typedef struct {
	uint32_t t;
	union {
		int32_t i;
		double d;
	};
} CFF_Value;

typedef struct {
	CFF_Value stack[256];
	CFF_Value transient[32];
	uint8_t index;
	uint8_t stem;
} CFF_Stack;

typedef struct {
	uint32_t op;
	uint32_t cnt;
	CFF_Value *vals;
} CFF_Dict_Entry;

typedef struct {
	uint32_t count;
	CFF_Dict_Entry *ents;
} CFF_Dict;

typedef struct {
	uint32_t count;
	CFF_Dict *fdarray;
	CFF_Dict *fprivate;
} CFF_Font_Dict;

typedef struct {
	double width;
	uint16_t cnt_contour;
	uint16_t cnt_point;
	uint16_t *c; // contour offset
	double *x;   // x position
	double *y;   // y position
	uint8_t *t;  // on or off
} CFF_Outline;

// blob and bloc of this CFF writer
typedef struct {
	uint32_t size;
	uint32_t free;
	uint32_t rank;
	uint8_t *data;
} cff_blob;

typedef struct {
	uint8_t *raw_data;
	uint32_t raw_length;
	uint16_t cnt_glyph;

	CFF_Header head;
	CFF_INDEX name;
	CFF_INDEX top_dict;
	CFF_INDEX string;
	CFF_INDEX global_subr;

	CFF_Encoding encodings; // offset
	CFF_Charset charsets;   // offset
	CFF_FDSelect fdselect;  // offset
	CFF_INDEX char_strings; // offset
	CFF_INDEX font_dict;    // offset
	CFF_INDEX local_subr;   // offset
} CFF_File;

// Outline builder method table
typedef struct {
	void (*setWidth)(void *context, float width);
	void (*newContour)(void *context);
	void (*lineTo)(void *context, float x1, float y1);
	void (*curveTo)(void *context, float x1, float y1, float x2, float y2, float x3, float y3);
	void (*setHint)(void *context, bool isVertical, float position, float width);
	void (*setMask)(void *context, bool isContourMask, bool *mask);
} cff_outline_builder_interface;

/*
  CFF -> Compact Font Format
  CS2 -> Type2 CharString
*/

extern char *op_cff_name(uint32_t op);
extern char *op_cs2_name(uint32_t op);

extern uint32_t decode_cff_token(uint8_t *start, CFF_Value *val);
extern uint32_t decode_cs2_token(uint8_t *start, CFF_Value *val);

// number, number, float
extern cff_blob *encode_cff_operator(int32_t val);
extern cff_blob *encode_cff_number(int32_t val);
extern cff_blob *encode_cff_real(double val);

// number, number, 16.16
extern cff_blob *encode_cs2_operator(int32_t val);
extern cff_blob *encode_cs2_number(int32_t val);
extern cff_blob *encode_cs2_real(double val);

/*
  Writer
*/
extern void blob_merge(cff_blob *dst, cff_blob *src);
extern void blob_free(cff_blob *b);

extern cff_blob *compile_header(void);
extern cff_blob *compile_name(const char *name);
extern cff_blob *compile_index(CFF_INDEX index);
extern cff_blob *compile_encoding(CFF_Encoding enc);
extern cff_blob *compile_charset(CFF_Charset cset);
extern cff_blob *compile_fdselect(CFF_FDSelect fd);
extern cff_blob *compile_topdict(CFF_INDEX top);
extern cff_blob *compile_private(CFF_File *f);
extern cff_blob *compile_fdarray(CFF_INDEX fdarray);
extern cff_blob *compile_outline(CFF_Outline *outline);
extern cff_blob *compile_outline_to_svg(CFF_Outline *outline);
extern cff_blob *compile_charstring(CFF_File *f);

extern void esrap_index(CFF_INDEX in);
extern void empty_index(CFF_INDEX *in);
extern void print_index(CFF_INDEX in);

cff_blob *compile_type2_value(double val);

extern char *get_cff_sid(uint16_t idx, CFF_INDEX str);
sds sdsget_cff_sid(uint16_t idx, CFF_INDEX str);

extern CFF_File *CFF_stream_open(uint8_t *data, uint32_t len);
extern CFF_File *CFF_file_open(const char *fname);
extern CFF_File *CFF_sfnt_open(const char *fname, uint32_t offset, uint32_t len);
extern void CFF_close(CFF_File *file);

extern uint8_t parse_subr(uint16_t idx, uint8_t *raw, CFF_INDEX fdarray, CFF_FDSelect select,
                          CFF_INDEX *subr);

extern CFF_Dict *parse_dict(uint8_t *data, uint32_t len);
extern void esrap_dict(CFF_Dict *d);
extern void print_dict(uint8_t *data, uint32_t len);
extern CFF_Value parse_dict_key(uint8_t *data, uint32_t len, uint32_t op, uint32_t idx);
extern CFF_Value lookup_dict(CFF_Dict *d, uint32_t op, uint32_t idx);

extern void print_glyph(uint8_t *data, uint32_t len, CFF_INDEX gsubr, CFF_INDEX lsubr,
                        CFF_Stack *stack);
extern void parse_outline(uint8_t *data, uint32_t len, CFF_INDEX gsubr, CFF_INDEX lsubr,
                          CFF_Stack *stack, CFF_Outline *outline);
void parse_outline_callback(uint8_t *data, uint32_t len, CFF_INDEX gsubr, CFF_INDEX lsubr,
                            CFF_Stack *stack, void *outline, cff_outline_builder_interface methods);

extern CFF_Outline *cff_outline_init(void);
extern void cff_outline_fini(CFF_Outline *out);
extern CFF_INDEX *cff_index_init(void);
extern void cff_index_fini(CFF_INDEX *out);

void cff_dict_callback(uint8_t *data, uint32_t len, void *context,
                       void (*callback)(uint32_t op, uint8_t top, CFF_Value *stack, void *context));

extern double cffnum(CFF_Value v);

#endif
