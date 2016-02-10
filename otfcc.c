#include <stdio.h>
#include <stdint.h>

typedef struct {
  uint32_t sfnt_version;
  uint16_t numTables;
  uint16_t searchRange;
  uint16_t entrySelector;
  uint16_t rangeShift;
} opentype_offset_table;

typedef struct {
  uint32_t tag;
  uint32_t checkSum;
  uint32_t offset;
  uint32_t length;
} opentype_table_record;

typedef struct {
  uint32_t TTCTag;
  uint32_t version;
  uint32_t numFonts;
  uint32_t*OffsetTable;
  uint32_t ulDsigTag;
  uint32_t ulDsigLength;
  uint32_t ulDsigOffset;
} opentype_collection_header;

typedef enum {
  TABLE_REQUIRED,
    // 'cmap', 'head', 'hhea', 'hmtx'
    // 'maxp', 'name', 'OS/2', 'post'
  TABLE_TRUETYPE,
    // 'cvt ', 'fpgm', 'glyf', 'loca'
    // 'prep', 'gasp'
  TABLE_POSTSCRIPT,
    // 'CFF ', 'VORG'
  TABLE_SVG,
    // 'SVG '
  TABLE_BITMAP,
    // 'EBDT', 'EBLC', 'EBSC', 'CBDT'
    // 'CBLT'
  TABLE_ADVANCED_TYPOGRAPHIC,
    // 'BASE', 'GDEF', 'GPOS', 'GSUB'
    // 'JSTF', 'MATH'
  TABLE_OTHER
    // 'DSIG', 'hdmx', 'kern', 'LTSH'
    // 'PLCT', 'VDMX', 'vhea', 'vmtx'
    // 'COLR', 'CPAL'
} opentype_table_type;

typedef struct {
  uint32_t table_version_number;
  uint32_t fontRevison;
  uint32_t checkSumAdjustment;
  uint32_t magicNumber;
  uint16_t flags;
  uint16_t unitsPerEm;
  int64_t  created;
  int64_t  modified;
  int16_t  xMin;
  int16_t  yMin;
  int16_t  xMax;
  int16_t  yMax;
  uint16_t macStyle;
  uint16_t lowestRecPPEM;
  int16_t  fontDirectoryHint;
  int16_t  indexToLocFormat;
  int16_t  glyphDataFormat;
} opentype_table_head;

typedef struct {
  uint32_t table_version_number;
  int16_t  Ascender;
  int16_t  Descender;
  int16_t  LineGap;
  uint16_t advanceWithMax;
  int16_t  minLeftSideBearing;
  int16_t  minRightSideBearing;
  int16_t  xMaxExtent;
  int16_t  caretSlopeRise;
  int16_t  caretSlopeRun;
  int16_t  dummy0;
  int16_t  dummy1;
  int16_t  dummy2;
  int16_t  dummy3;
  int16_t  metricDataFormat;
  uint16_t numberOfMetrics;
} opentype_table_hhea;

typedef struct {
  uint16_t version;
  int16_t  xAvgCharWidth;
  uint16_t usWeightClass;
  uint16_t usWidthClass;
  uint16_t fsType;
  int16_t  ySubscriptXSize;
  int16_t  ySubscriptYSize;
  int16_t  ySubscriptXOffset;
  int16_t  ySubscriptYOffset;
  int16_t  ySupscriptXSize;
  int16_t  ySupscriptYSize;
  int16_t  ySupscriptXOffset;
  int16_t  ySupscriptYOffset;
  int16_t  yStrikeoutSize;
  int16_t  yStrikeoutPosition;
  int16_t  sFamilyClass;
  uint8_t  panose[10];
  uint32_t ulUnicodeRange1;
  uint32_t ulUnicodeRange2;
  uint32_t ulUnicodeRange3;
  uint32_t ulUnicodeRange4;
  int8_t   achVendID[4];
  uint16_t fsSelection;
  uint16_t usFirstCharIndex;
  uint16_t usLastCharIndex;
  int16_t  sTypoAscender;
  int16_t  sTypoDescender;
  uint32_t ulCodePageRange1;
  uint32_t ulCodePageRange2;
  int16_t  sxHeight;
  int16_t  sCapHeight;
  uint16_t usDefaultCHar;
  uint16_t usBreakChar;
  uint16_t usMaxContext;
  uint16_t usLowerOpticalPointSize;
  uint16_t usUpperOpticalPointSize;
} opentype_table_os2;

int main (void)
{
  return 0;
}

