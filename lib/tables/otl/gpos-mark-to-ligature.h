#ifndef CARYLL_TABLES_OTL_GPOS_MARK_TO_LIGATURE_H
#define CARYLL_TABLES_OTL_GPOS_MARK_TO_LIGATURE_H

#include "otl.h"

void delete_lig_attachment(mark_to_ligature_base *att);

void caryll_delete_gpos_mark_to_ligature(otl_lookup *lookup);
otl_subtable *caryll_read_gpos_mark_to_ligature(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset);
json_value *caryll_gpos_mark_to_ligature_to_json(otl_subtable *st);
otl_subtable *caryll_gpos_mark_to_ligature_from_json(json_value *_subtable);
caryll_buffer *caryll_write_gpos_mark_to_ligature(otl_subtable *_subtable);
#endif
