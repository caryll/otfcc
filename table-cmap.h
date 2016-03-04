
typedef struct {
  uint16_t version;
  uint16_t numTables;
}table_cmap;

// Platform ID    
// 0  Unicode     
// 1  Macintosh   
// 2  (reserved)  
// 3  Microsoft   

//    Unicode   Windows
// 0            Symbol
// 1            UCS-2
// 2  ISO 10646 SJIS
// 3  BMP       PRC
// 4  non-BMP   Big5
// 5  UVS       Wansung
// 6  Full      Johab
// 7            
// 8            
// 9            
// 10           UCS-4

typedef struct {
  uint16_t platformID;
  uint16_t encodingID;
  uint32_t offset;
} opentype_encoding_record;

typedef struct {
  uint16_t format;
  uint16_t length;
  uint16_t language;
  uint8_t  glyphIdArray[256];
} subtable_format0;

typedef struct {
  uint16_t firstCode;
  uint16_t entryCount;
  int16_t  idDelta;
  uint16_t idRangeOffset;
} opentype_subheader;

typedef struct {
  uint16_t format;
  uint16_t length;
  uint16_t language;
  uint16_t subHeaderKeys[256];
  opentype_subheader * subHeaders;
  uint16_t * glyphIndexArray;
} subtable_format2;

typedef struct {
  uint16_t format;
  uint16_t length;
  uint16_t language;
  uint16_t segCountX2;
  uint16_t searchRange;
  uint16_t entrySelector;
  uint16_t rangeShift;
  uint16_t * endCount;
  uint16_t reservedPad;
  uint16_t * startCount;
  int16_t  idDelta;
  uint16_t * idRangeOffset;
  uint16_t * glyphIdArray;
} opentype_subtable_format4;

typedef struct {
  uint16_t format;
  uint16_t length;
  uint16_t language;
  uint16_t firstCode;
  uint16_t entryCount;
  uint16_t * glyphIdArray;
} subtable_format6;

typedef struct {
  uint32_t startCharCode;
  uint32_t endCharCode;
  uint32_t startGlyphID;
} opentype_groups;

typedef struct {
  uint16_t format;
  uint16_t reserved;
  uint32_t length;
  uint32_t language;
  uint8_t  is32[8192];
  uint32_t nGroups;
} subtable_format8;

typedef struct {
  uint16_t format;
  uint16_t reserved;
  uint32_t length;
  uint32_t language;
  uint32_t startCharCode;
  uint32_t numChars;
  uint16_t glyphs;
} subtable_format10;

typedef struct {
  uint16_t format;
  uint16_t reserved;
  uint32_t length;
  uint32_t language;
  uint32_t nGroups;
} subtable_format12;

typedef struct {
  uint16_t format;
  uint16_t reserved;
  uint32_t length;
  uint32_t language;
  uint32_t nGroups;
} subtable_format13;

typedef struct {
  uint16_t format;
  uint16_t length;
  uint32_t numVarSelectorRecords;
} subtable_format14;

typedef struct {
  uint32_t varSelector;
  uint32_t defaultUVSOffset;
  uint32_t nonDefaultUVSOffset;
} opentype_num_var_selector_record;

typedef struct {
  uint32_t numUnicodeValueRanges;
} opentype_default_uvs_table;

typedef struct {
  uint32_t startUnicodeValue;
  uint8_t additionalCount;
} opentype_num_unicode_value_ranges;

typedef struct {
  uint32_t numUVSMappings;
} opentype_non_default_uvs_table;

typedef struct {
  uint32_t unicodeValue;
  uint16_t glyphID;
} opentype_num_uvs_mappings;

