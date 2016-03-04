
typedef struct {
  uint16_t advanceWidth;
  int16_t  lsb;
} horizontal_metric;

typedef struct {
  // Horizontal metrics
  opentype_horizontal_metric * metrics;
  int16_t * leftSideBearing;
} table_hmtx;
