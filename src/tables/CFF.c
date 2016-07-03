#include "CFF.h"

table_cff *caryll_read_cff(caryll_packet packet, uint32_t tag) {
  return NULL;
}

void caryll_delete_cff(table_cff *table) {
  //
}

void caryll_cff_to_json(table_cff *table, json_value *root, caryll_dump_options *dumpopts,
const char *tag) {
  //
}

table_cff *caryll_cff_from_json(json_value *root, const tag *tag) {
  return NULL;
}

caryll_buffer *caryll_write_cff(table *table) {
  //
  return NULL;
}
