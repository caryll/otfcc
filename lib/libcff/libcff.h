#ifndef CFF_DATA_TYPES
#define CFF_DATA_TYPES

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/util.h>

#include "cff-util.h"
#include "cff-value.h"
#include "cff-index.h"
#include "cff-dict.h"
#include "cff-charset.h"
#include "cff-fdselect.h"

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

typedef struct {
	CFF_Value stack[256];
	CFF_Value transient[32];
	uint8_t index;
	uint8_t stem;
} CFF_Stack;

typedef struct {
	uint8_t *raw_data;
	uint32_t raw_length;
	uint16_t cnt_glyph;

	CFF_Header head;
	CFF_Index name;
	CFF_Index top_dict;
	CFF_Index string;
	CFF_Index global_subr;

	CFF_Encoding encodings; // offset
	CFF_Charset charsets;   // offset
	CFF_FDSelect fdselect;  // offset
	CFF_Index char_strings; // offset
	CFF_Index font_dict;    // offset
	CFF_Index local_subr;   // offset
} CFF_File;

// Outline builder method table
typedef struct {
	void (*setWidth)(void *context, float width);
	void (*newContour)(void *context);
	void (*lineTo)(void *context, float x1, float y1);
	void (*curveTo)(void *context, float x1, float y1, float x2, float y2, float x3, float y3);
	void (*setHint)(void *context, bool isVertical, float position, float width);
	void (*setMask)(void *context, bool isContourMask, bool *mask);
	double (*getrand)(void *context);
} cff_outline_builder_interface;

/*
  CFF -> Compact Font Format
  CS2 -> Type2 CharString
*/

extern char *op_cff_name(uint32_t op);
extern char *op_cs2_name(uint32_t op);
uint8_t cs2_op_standard_arity(uint32_t op);

sds sdsget_cff_sid(uint16_t idx, CFF_Index str);

extern uint32_t decode_cff_token(uint8_t *start, CFF_Value *val);
extern uint32_t decode_cs2_token(uint8_t *start, CFF_Value *val);

// number, number, float
extern caryll_buffer *encode_cff_operator(int32_t val);
extern caryll_buffer *encode_cff_number(int32_t val);
extern caryll_buffer *encode_cff_real(double val);

/*
  Writer
*/

extern caryll_buffer *compile_offset(int32_t val);
extern caryll_buffer *compile_header(void);
extern caryll_buffer *compile_encoding(CFF_Encoding enc);

void merge_cs2_operator(caryll_buffer *blob, int32_t val);
void merge_cs2_operand(caryll_buffer *blob, double val);
void merge_cs2_special(caryll_buffer *blob, uint8_t val);

extern uint8_t parse_subr(uint16_t idx, uint8_t *raw, CFF_Index fdarray, CFF_FDSelect select,
                          CFF_Index *subr);
void parse_outline_callback(uint8_t *data, uint32_t len, CFF_Index gsubr, CFF_Index lsubr,
                            CFF_Stack *stack, void *outline, cff_outline_builder_interface methods);

// File
extern CFF_File *CFF_stream_open(uint8_t *data, uint32_t len);
extern void CFF_close(CFF_File *file);

#endif
