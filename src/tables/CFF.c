
enum {
  // One-byte CFF DICT Operators
  op_version = 0x00, op_Notice = 0x01,
  op_FullName = 0x02, op_FamilyName = 0x03,
  op_Weight = 0x04, op_FontBBox = 0x05,
  op_BlueValues = 0x06, op_OtherBlues = 0x07,
  op_FamilyBlues = 0x08, op_FamilyOtherBlues = 0x09,
  op_StdHW = 0x0a, op_StdVW = 0x0b,
  op_UniqueID = 0x0d, op_XUID = 0x0e,
  op_charset = 0x0f, op_Encoding = 0x10,
  op_CharStrings = 0x11, op_Private = 0x12,
  op_Subrs = 0x13, op_defaultWidthX = 0x14,
  op_nominalWidthX = 0x15, op_shortint = 0x1c,
  op_longint = 0x1d, op_BCD = 0x1e,
  // Two-byte CFF DICT Operators
  op_Copyright = 0x0c00, op_isFixedPitch = 0x0c01,
  op_ItalicAngle = 0x0c02, op_UnderlinePosition = 0x0c03,
  op_UnderlineThickness = 0x0c04, op_PaintType = 0x0c05,
  op_CharstringType = 0x0c06, op_FontMatrix = 0x0c07,
  op_StrokeWidth = 0x0c08, op_BlueScale = 0x0c09,
  op_BlueShift = 0x0c0a, op_BlueBuzz = 0x0c0b,
  op_StemSnapH = 0x0c0c, op_StemSnapV = 0x0c0d,
  op_ForceBold = 0x0c0e, op_LanguageGroup = 0x0c11,
  op_ExpansionFactor = 0x0c12, op_initialRandomSeed = 0x0c13,
  op_SyntheicBase = 0x0c14, op_PostScript = 0x0c15,
  op_BaseFontName = 0x0c16, op_BaseFontBlend = 0x0c17,
  op_ROS = 0x0c1e, op_CIDFontVersion = 0x0c1f,
  op_CIDFontRevision = 0x0c20, op_CIDFontType = 0x0c21,
  op_CIDCount = 0x0c22, op_UIDBase = 0x0c23,
  op_FDArray = 0x0c24, op_FDSelect = 0x0c25,
  op_FontName = 0x0c26,
};

enum {
  // One-byte Type 2 Operators
  op_hstem = 0x01, op_vstem = 0x03,
  op_vmoveto = 0x04, op_rlineto = 0x05,
  op_hlineto = 0x06, op_vlineto = 0x07,
  op_rrcurveto = 0x08, op_callsubr = 0x0a,
  op_return = 0x0b, op_endchar = 0x0e,
  op_hstemhm = 0x12, op_hintmask = 0x13,
  op_cntmask = 0x14, op_rmoveto = 0x15,
  op_hmoveto = 0x16, op_vstemhm = 0x17,
  op_rcurveline = 0x18, op_rlinecurve = 0x19,
  op_vvcurveto = 0x1a, op_callgsubr = 0x1d,
  op_vhcurveto = 0x1e, op_hvcurveto = 0x1f,
  // Two-byte Type 2 Operators
  op_and = 0x0c03, op_or = 0x0c04,
  op_not = 0x0c05, op_abs = 0x0c09,
  op_add = 0x0c0a, op_sub = 0x0c0b,
  op_div = 0x0c0c, op_neg = 0x0c0e,
  op_eq = 0x0c0f, op_drop = 0x0c12,
  op_put = 0x0c14, op_get = 0x0c15,
  op_ifelse = 0x0c16, op_random = 0x0c17,
  op_mul = 0x0c18, op_sqrt = 0x0c1a,
  op_dup = 0x0c1b, op_exch = 0x0c1c,
  op_index = 0x0c1d, op_roll = 0x0c1e,
  op_hflex = 0x0c22, op_flex = 0x0c23,
  op_hflex1 = 0x0c24, op_flex1 = 0x0c25,
};

typedef struct {
  uint16_t   count;
  uint8_t    offSize;
  uint32_t * offset;
  uint8_t *  data;
} CFF_INDEX;

typedef struct {
  uint8_t major;
  uint8_t minor;
  uint8_t hdrSize;
  uint8_t offSize;
} CFF_Header;

typedef struct {
  CFF_Header head;
  CFF_INDEX  name;
  CFF_INDEX  top_dict;
  CFF_INDEX  string;
  CFF_INDEX  global_subrs;
  // I AM GOING TO BUY BREAD.
} CFF_File;

