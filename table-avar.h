
typedef struct {
  shortFrac_t fromCoord;
  shortFrac_t toCoord;
} shortFracCorrespondence_t;

typedef struct {
  uint16_t pairCount;
  shortFracCorrespondence_t correspondence;
} shortFracSegment_t;

typedef struct {
  fixed32_t version;
  int32_t   axisCount;
  shortFracSegment_t segment;
} table_avar;
