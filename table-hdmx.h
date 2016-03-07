#pragma once

#include <stdint.h>

typedef struct {
  uint8_t pixelSize;
  uint8_t maxWidth;
  uint8_t * widths;
} device_record;

typedef struct {
  // Horizontal device metrics
  uint16_t version;
  int16_t  numRecords;
  int32_t  sizeDeviceRecord;
  device_record * records;
} table_hdmx;