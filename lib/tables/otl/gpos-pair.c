#include "gpos-pair.h"
#include "gpos-common.h"

void delete_otl_gpos_pair_subtable(otl_subtable *_subtable) {
	if (_subtable) {
		subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
		if (subtable->coverage) caryll_delete_coverage(subtable->coverage);
		if (subtable->firstValues) {
			for (uint16_t j = 0; j <= subtable->first->maxclass; j++) {
				free(subtable->firstValues[j]);
			}
			free(subtable->firstValues);
		}
		if (subtable->secondValues) {
			for (uint16_t j = 0; j <= subtable->first->maxclass; j++) {
				free(subtable->secondValues[j]);
			}
			free(subtable->secondValues);
		}
		caryll_delete_classdef(subtable->first);
		caryll_delete_classdef(subtable->second);
		free(_subtable);
	}
}
void caryll_delete_gpos_pair(otl_lookup *lookup) {
	if (lookup) {
		if (lookup->subtables) {
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				delete_otl_gpos_pair_subtable(lookup->subtables[j]);
			free(lookup->subtables);
		}
		free(lookup);
	}
}

typedef struct {
	int gid;
	int cid;
	UT_hash_handle hh;
} pair_classifier_hash;

otl_subtable *caryll_read_gpos_pair(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_subtable *_subtable;
	NEW(_subtable);
	subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	subtable->coverage = NULL;
	subtable->first = NULL;
	subtable->second = NULL;
	subtable->firstValues = NULL;
	subtable->secondValues = NULL;

	checkLength(offset + 2);
	uint16_t subtableFormat = read_16u(data + offset);
	if (subtableFormat == 1) {
		// pair adjustment by individuals
		otl_coverage *cov = caryll_read_coverage(data, tableLength, offset + read_16u(data + offset + 2));
		NEW(subtable->first);
		subtable->first->numGlyphs = cov->numGlyphs;
		subtable->first->maxclass = cov->numGlyphs - 1;
		subtable->first->glyphs = cov->glyphs;
		NEW_N(subtable->first->classes, cov->numGlyphs);
		for (uint16_t j = 0; j < cov->numGlyphs; j++) {
			subtable->first->classes[j] = j;
		}
		free(cov);

		uint16_t format1 = read_16u(data + offset + 4);
		uint16_t format2 = read_16u(data + offset + 6);
		uint8_t len1 = position_format_length(format1);
		uint8_t len2 = position_format_length(format2);

		uint16_t pairSetCount = read_16u(data + offset + 8);
		if (pairSetCount != subtable->first->numGlyphs) goto FAIL;
		checkLength(offset + 10 + 2 * pairSetCount);

		// pass 1: check length
		for (uint16_t j = 0; j < pairSetCount; j++) {
			uint32_t pairSetOffset = offset + read_16u(data + offset + 10 + 2 * j);
			checkLength(pairSetOffset + 2);
			uint16_t pairCount = read_16u(data + pairSetOffset);
			checkLength(pairSetOffset + 2 + (2 + len1 + len2) * pairCount);
		}
		// pass 2: make hashtable about second glyphs to construct a classdec
		pair_classifier_hash *h = NULL;
		for (uint16_t j = 0; j < pairSetCount; j++) {
			uint32_t pairSetOffset = offset + read_16u(data + offset + 10 + 2 * j);
			uint16_t pairCount = read_16u(data + pairSetOffset);
			for (uint16_t k = 0; k < pairCount; k++) {
				int second = read_16u(data + pairSetOffset + 2 + (2 + len1 + len2) * k);
				pair_classifier_hash *s;
				HASH_FIND_INT(h, &second, s);
				if (!s) {
					NEW(s);
					s->gid = second;
					s->cid = HASH_COUNT(h) + 1;
					HASH_ADD_INT(h, gid, s);
				}
			}
		}
		// pass 3: make the matrix
		NEW(subtable->second);
		subtable->second->numGlyphs = HASH_COUNT(h);
		subtable->second->maxclass = HASH_COUNT(h);
		NEW_N(subtable->second->classes, subtable->second->numGlyphs);
		NEW_N(subtable->second->glyphs, subtable->second->numGlyphs);
		uint16_t class2Count = subtable->second->maxclass + 1;

		NEW_N(subtable->firstValues, subtable->first->maxclass + 1);
		NEW_N(subtable->secondValues, subtable->first->maxclass + 1);
		for (uint16_t j = 0; j <= subtable->first->maxclass; j++) {
			NEW_N(subtable->firstValues[j], class2Count);
			NEW_N(subtable->secondValues[j], class2Count);
			for (uint16_t k = 0; k < class2Count; k++) {
				subtable->firstValues[j][k] = position_zero();
				subtable->secondValues[j][k] = position_zero();
			}
		};
		for (uint16_t j = 0; j <= subtable->first->maxclass; j++) {
			uint32_t pairSetOffset = offset + read_16u(data + offset + 10 + 2 * j);
			uint16_t pairCount = read_16u(data + pairSetOffset);
			for (uint16_t k = 0; k < pairCount; k++) {
				int second = read_16u(data + pairSetOffset + 2 + (2 + len1 + len2) * k);
				pair_classifier_hash *s;
				HASH_FIND_INT(h, &second, s);
				if (s) {
					subtable->firstValues[j][s->cid] =
					    read_gpos_value(data, tableLength, pairSetOffset + 2 + (2 + len1 + len2) * k + 2, format1);
					subtable->secondValues[j][s->cid] = read_gpos_value(
					    data, tableLength, pairSetOffset + 2 + (2 + len1 + len2) * k + 2 + len1, format2);
				}
			}
		}
		// pass 4: return
		pair_classifier_hash *s, *tmp;
		uint16_t jj = 0;
		HASH_ITER(hh, h, s, tmp) {
			subtable->second->glyphs[jj] = handle_from_id(s->gid);
			subtable->second->classes[jj] = s->cid;
			jj++;
			HASH_DEL(h, s);
			free(s);
		}
		return _subtable;
	} else if (subtableFormat == 2) {
		// pair adjustment by classes
		checkLength(offset + 16);
		uint16_t format1 = read_16u(data + offset + 4);
		uint16_t format2 = read_16u(data + offset + 6);
		uint8_t len1 = position_format_length(format1);
		uint8_t len2 = position_format_length(format2);
		otl_coverage *cov = caryll_read_coverage(data, tableLength, offset + read_16u(data + offset + 2));
		subtable->first = caryll_read_classdef(data, tableLength, offset + read_16u(data + offset + 8));
		subtable->first = caryll_expand_classdef(cov, subtable->first);
		caryll_delete_coverage(cov);
		subtable->second = caryll_read_classdef(data, tableLength, offset + read_16u(data + offset + 10));
		if (!subtable->first || !subtable->second) goto FAIL;
		uint16_t class1Count = read_16u(data + offset + 12);
		uint16_t class2Count = read_16u(data + offset + 14);
		if (subtable->first->maxclass + 1 != class1Count) goto FAIL;
		if (subtable->second->maxclass + 1 != class2Count) goto FAIL;

		checkLength(offset + 16 + class1Count * class2Count * (len1 + len2));
		// read the matrix
		NEW_N(subtable->firstValues, class1Count);
		NEW_N(subtable->secondValues, class1Count);

		for (uint16_t j = 0; j < class1Count; j++) {
			NEW_N(subtable->firstValues[j], class2Count);
			NEW_N(subtable->secondValues[j], class2Count);
			for (uint16_t k = 0; k < class2Count; k++) {
				subtable->firstValues[j][k] =
				    read_gpos_value(data, tableLength, offset + 16 + (j * class2Count + k) * (len1 + len2), format1);
				subtable->secondValues[j][k] = read_gpos_value(
				    data, tableLength, offset + 16 + (j * class2Count + k) * (len1 + len2) + len1, format2);
			}
		}

		return _subtable;
	}
FAIL:
	delete_otl_gpos_pair_subtable(_subtable);
	return NULL;
}

json_value *caryll_gpos_pair_to_json(otl_subtable *_subtable) {
	subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	json_value *st = json_object_new(3);
	json_object_push(st, "first", caryll_classdef_to_json(subtable->first));
	json_object_push(st, "second", caryll_classdef_to_json(subtable->second));

	json_value *mat = json_array_new(subtable->first->maxclass + 1);
	for (uint16_t j = 0; j <= subtable->first->maxclass; j++) {
		json_value *row = json_array_new(subtable->second->maxclass + 1);
		for (uint16_t k = 0; k <= subtable->second->maxclass; k++) {
			uint8_t f1 = required_position_format(subtable->firstValues[j][k]);
			uint8_t f2 = required_position_format(subtable->secondValues[j][k]);
			if (f1 | f2) {
				if (f1 == FORMAT_DWIDTH && f2 == 0) {
					// simple kerning
					json_array_push(row, json_integer_new(subtable->firstValues[j][k].dWidth));
				} else {
					json_value *pair = json_object_new(2);
					if (f1) { json_object_push(pair, "first", gpos_value_to_json(subtable->firstValues[j][k])); }
					if (f2) { json_object_push(pair, "second", gpos_value_to_json(subtable->secondValues[j][k])); }
					json_array_push(row, pair);
				}
			} else {
				json_array_push(row, json_integer_new(0));
			}
		}
		json_array_push(mat, preserialize(row));
	}
	json_object_push(st, "matrix", mat);
	return st;
}
otl_subtable *caryll_gpos_pair_from_json(json_value *_subtable) {
	otl_subtable *_st;
	NEW(_st);
	subtable_gpos_pair *subtable = &(_st->gpos_pair);
	subtable->coverage = NULL;
	subtable->first = NULL;
	subtable->second = NULL;
	subtable->firstValues = NULL;
	subtable->secondValues = NULL;

	json_value *_mat = json_obj_get_type(_subtable, "matrix", json_array);
	subtable->first = caryll_classdef_from_json(json_obj_get_type(_subtable, "first", json_object));
	subtable->second = caryll_classdef_from_json(json_obj_get_type(_subtable, "second", json_object));
	if (!_mat || !subtable->first || !subtable->second) goto FAIL;

	uint16_t class1Count = subtable->first->maxclass + 1;
	uint16_t class2Count = subtable->second->maxclass + 1;

	NEW_N(subtable->firstValues, class1Count);
	NEW_N(subtable->secondValues, class1Count);

	for (uint16_t j = 0; j < class1Count; j++) {
		NEW_N(subtable->firstValues[j], class2Count);
		NEW_N(subtable->secondValues[j], class2Count);
		for (uint16_t k = 0; k < class2Count; k++) {
			subtable->firstValues[j][k] = position_zero();
			subtable->secondValues[j][k] = position_zero();
		}
	}
	for (uint16_t j = 0; j < class1Count && j < _mat->u.array.length; j++) {
		json_value *_row = _mat->u.array.values[j];
		if (!_row || _row->type != json_array) continue;
		for (uint16_t k = 0; k < class2Count && k < _row->u.array.length; k++) {
			json_value *_item = _row->u.array.values[k];
			if (_item->type == json_integer) {
				subtable->firstValues[j][k].dWidth = _item->u.integer;
			} else if (_item->type == json_double) {
				subtable->firstValues[j][k].dWidth = _item->u.dbl;
			} else if (_item->type == json_object) {
				subtable->firstValues[j][k] = gpos_value_from_json(json_obj_get(_item, "first"));
				subtable->secondValues[j][k] = gpos_value_from_json(json_obj_get(_item, "second"));
			}
		}
	}
	return _st;
FAIL:
	delete_otl_gpos_pair_subtable(_st);
	return NULL;
}

caryll_buffer *caryll_write_gpos_pair_individual(otl_subtable *_subtable) {
	subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	uint16_t format1 = 0;
	uint16_t format2 = 0;
	uint16_t class1Count = subtable->first->maxclass + 1;
	uint16_t class2Count = subtable->second->maxclass + 1;

	for (uint16_t j = 0; j < class1Count; j++) {
		for (uint16_t k = 0; k < class2Count; k++) {
			format1 |= required_position_format(subtable->firstValues[j][k]);
			format2 |= required_position_format(subtable->secondValues[j][k]);
		}
	}
	uint16_t *pairCounts;
	NEW_N(pairCounts, subtable->first->numGlyphs);
	for (uint16_t j = 0; j < subtable->first->numGlyphs; j++) {
		pairCounts[j] = 0;
		for (uint16_t k = 0; k < subtable->second->numGlyphs; k++) {
			uint16_t c1 = subtable->first->classes[j];
			uint16_t c2 = subtable->second->classes[k];
			if (required_position_format(subtable->firstValues[c1][c2]) |
			    required_position_format(subtable->secondValues[c1][c2])) {
				pairCounts[j] += 1;
			}
		}
	}

	caryll_bkblock *root =
	    new_bkblock(b16, 1,                                                                  // PosFormat
	                p16, new_bkblock_from_buffer(caryll_write_coverage(subtable->coverage)), // Coverage
	                b16, format1,                                                            // ValueFormat1
	                b16, format2,                                                            // ValueFormat2
	                b16, subtable->first->numGlyphs,                                         // PairSetCount
	                bkover);
	for (uint16_t j = 0; j < subtable->first->numGlyphs; j++) {
		caryll_bkblock *pairSet = new_bkblock(b16, pairCounts[j], // PairValueCount
		                                      bkover);
		for (uint16_t k = 0; k < subtable->second->numGlyphs; k++) {
			uint16_t c1 = subtable->first->classes[j];
			uint16_t c2 = subtable->second->classes[k];
			if (required_position_format(subtable->firstValues[c1][c2]) |
			    required_position_format(subtable->secondValues[c1][c2])) {
				bkblock_push(pairSet, b16, subtable->second->glyphs[k].index,                // SecondGlyph
				             bembed, bk_gpos_value(subtable->firstValues[c1][c2], format1),  // Value1
				             bembed, bk_gpos_value(subtable->secondValues[c1][c2], format2), // Value2
				             bkover);
			}
		}
		bkblock_push(root, p16, pairSet, bkover);
	}
	return caryll_write_bk(root);
}
caryll_buffer *caryll_write_gpos_pair_classes(otl_subtable *_subtable) {
	subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	uint16_t format1 = 0;
	uint16_t format2 = 0;
	uint16_t class1Count = subtable->first->maxclass + 1;
	uint16_t class2Count = subtable->second->maxclass + 1;

	for (uint16_t j = 0; j < class1Count; j++) {
		for (uint16_t k = 0; k < class2Count; k++) {
			format1 |= required_position_format(subtable->firstValues[j][k]);
			format2 |= required_position_format(subtable->secondValues[j][k]);
		}
	}
	caryll_bkblock *root =
	    new_bkblock(b16, 2,                                                                  // PosFormat
	                p16, new_bkblock_from_buffer(caryll_write_coverage(subtable->coverage)), // Coverage
	                b16, format1,                                                            // ValueFormat1
	                b16, format2,                                                            // ValueFormat2
	                p16, new_bkblock_from_buffer(caryll_write_classdef(subtable->first)),    // ClassDef1
	                p16, new_bkblock_from_buffer(caryll_write_classdef(subtable->second)),   // ClassDef2
	                b16, class1Count,                                                        // Class1Count
	                b16, class2Count,                                                        // Class2Count
	                bkover);
	for (uint16_t j = 0; j < class1Count; j++) {
		for (uint16_t k = 0; k < class2Count; k++) {
			bkblock_push(root, bembed, bk_gpos_value(subtable->firstValues[j][k], format1), // Value1
			             bembed, bk_gpos_value(subtable->secondValues[j][k], format2),      // Value2
			             bkover);
		}
	}
	return caryll_write_bk(root);
}
caryll_buffer *caryll_write_gpos_pair(otl_subtable *_subtable) {
	caryll_buffer *format1 = caryll_write_gpos_pair_individual(_subtable);
	caryll_buffer *format2 = caryll_write_gpos_pair_classes(_subtable);
	if (buflen(format1) < buflen(format2)) {
		buffree(format2);
		return format1;
	} else {
		buffree(format1);
		return format2;
	}
}
