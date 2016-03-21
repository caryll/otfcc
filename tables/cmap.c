#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void encode(cmap_hash *map, int c, uint16_t gid) {
	cmap_entry *s;
	HASH_FIND_INT(*map, &c, s);
	if (s == NULL) {
		s = malloc(sizeof(cmap_entry));
		s->glyph.gid = gid;
		s->glyph.name = NULL;
		s->unicode = c;
		HASH_ADD_INT(*map, unicode, s);
	}
}

void caryll_read_format_12(font_file_pointer start, uint32_t lengthLimit, cmap_hash *map) {
	if (lengthLimit < 16) return;
	uint32_t nGroups = caryll_blt32u(start + 12);
	if (lengthLimit < 16 + 12 * nGroups) return;
	for (uint32_t j = 0; j < nGroups; j++) {
		uint32_t startCode = caryll_blt32u(start + 16 + 12 * j);
		uint32_t endCode = caryll_blt32u(start + 16 + 12 * j + 4);
		uint32_t startGID = caryll_blt32u(start + 16 + 12 * j + 8);
		for (uint32_t c = startCode; c <= endCode; c++) {
			encode(map, c, (c - startCode) + startGID);
		}
	}
}

void caryll_read_format_4(font_file_pointer start, uint32_t lengthLimit, cmap_hash *map) {
	if (lengthLimit < 14) return;
	uint16_t segmentsCount = caryll_blt16u(start + 6) / 2;
	if (lengthLimit < 16 + segmentsCount * 8) return;
	for (uint16_t j = 0; j < segmentsCount; j++) {
		uint16_t endCode = caryll_blt16u(start + 14 + j * 2);
		uint16_t startCode = caryll_blt16u(start + 14 + segmentsCount * 2 + 2 + j * 2);
		int16_t idDelta = caryll_blt16u(start + 14 + segmentsCount * 4 + 2 + j * 2);
		uint32_t idRangeOffsetOffset = 14 + segmentsCount * 6 + 2 + j * 2;
		uint16_t idRangeOffset = caryll_blt16u(start + idRangeOffsetOffset);
		if (startCode < 0xFFFF) {
			if (idRangeOffset == 0) {
				for (uint16_t c = startCode; c <= endCode; c++) {
					uint16_t gid = (c + idDelta) & 0xFFFF;
					if (c != 0xFFFF) { encode(map, c, gid); }
				}
			} else {
				for (uint16_t c = startCode; c <= endCode; c++) {
					uint32_t glyphOffset = idRangeOffset + (c - startCode) * 2 + idRangeOffsetOffset;
					if (glyphOffset + 2 >= lengthLimit) continue; // ignore this encoding slot when o-o-r

					uint16_t gid = (caryll_blt16u(start + glyphOffset) + idDelta) & 0xFFFF;
					if (c != 0xFFFF) { encode(map, c, gid); }
				}
			}
		}
	}
}

void caryll_read_mapping_table(font_file_pointer start, uint32_t lengthLimit, cmap_hash *map) {
	uint16_t format = caryll_blt16u(start);
	if (format == 4) {
		caryll_read_format_4(start, lengthLimit, map);
	} else if (format == 12) {
		caryll_read_format_12(start, lengthLimit, map);
	}
}

int by_unicode(cmap_entry *a, cmap_entry *b) {
	return (a->unicode - b->unicode);
}

// OTFCC will not support all `cmap` mappings.
void caryll_read_cmap(caryll_font *font, caryll_packet packet) {
	// the map is a reference to a hash table
	cmap_hash *map = NULL;
	FOR_TABLE('cmap', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 4) goto CMAP_CORRUPTED;

		map = malloc(sizeof(cmap_hash));
		*map = NULL; // intialize to empty hashtable
		uint16_t numTables = caryll_blt16u(data + 2);
		if (length < 4 + 8 * numTables) goto CMAP_CORRUPTED;
		for (uint16_t j = 0; j < numTables; j++) {
			uint16_t platform = caryll_blt16u(data + 4 + 8 * j);
			uint16_t encoding = caryll_blt16u(data + 4 + 8 * j + 2);
			if (platform == 0 || (platform == 3 && encoding == 1) || (platform == 3 && encoding == 10)) {
				uint32_t tableOffset = caryll_blt32u(data + 4 + 8 * j + 4);
				caryll_read_mapping_table(data + tableOffset, length - tableOffset, map);
			}
		};
		HASH_SORT(*map, by_unicode);
		font->cmap = map;
	}
	return;
CMAP_CORRUPTED:
	printf("table 'cmap' corrupted.\n");
	if (map != NULL) free(map);
	return;
}

void caryll_delete_table_cmap(caryll_font *font) {
	cmap_entry *s, *tmp;
	HASH_ITER(hh, *(font->cmap), s, tmp) {
		// delete and free all cmap entries
		s->glyph.name = NULL;
		HASH_DEL(*(font->cmap), s);
		free(s);
	}
	free(font->cmap);
}

void caryll_cmap_to_json(caryll_font *font, JSON_Object *root) {
	if(!font->cmap) return;
	JSON_Value *_cmapObj = json_value_init_object();
	JSON_Object *cmapObj = json_value_get_object(_cmapObj);
	
	cmap_entry *item;
	foreach_hash(item, *font->cmap) {
		sds key = sdsfromlonglong(item->unicode);
		json_object_set_string(cmapObj, key, item->glyph.name);
		sdsfree(key);
	}
	json_object_set_value(root, "cmap", _cmapObj);
}
