
typedef struct {
  uint16_t platformID;
  uint16_t encodingID;
  uint16_t languageID;
  uint16_t nameID;
  uint16_t length;
  uint16_t offset;
} name_record;

typedef struct {
  uint16_t length;
  uint16_t offset;
} lang_tag_record;

typedef struct {
  uint16_t format;
  uint16_t count;
  uint16_t stringOffset;
  name_record*nameRecord;
  uint16_t langTagCount;
  lang_tag_record*langTagRecord;
} table_name;

