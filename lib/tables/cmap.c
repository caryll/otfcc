#include "cmap.h"

static void encode(table_cmap *map, int c, uint16_t gid) {
	cmap_Entry *s;
	HASH_FIND_INT(*map, &c, s);
	if (s == NULL) {
		s = malloc(sizeof(cmap_Entry));
		s->glyph = handle_fromIndex(gid);
		s->unicode = c;
		HASH_ADD_INT(*map, unicode, s);
	}
}

static void caryll_read_format_12(font_file_pointer start, uint32_t lengthLimit, table_cmap *map) {
	if (lengthLimit < 16) return;
	uint32_t nGroups = read_32u(start + 12);
	if (lengthLimit < 16 + 12 * nGroups) return;
	for (uint32_t j = 0; j < nGroups; j++) {
		uint32_t startCode = read_32u(start + 16 + 12 * j);
		uint32_t endCode = read_32u(start + 16 + 12 * j + 4);
		uint32_t startGID = read_32u(start + 16 + 12 * j + 8);
		for (uint32_t c = startCode; c <= endCode; c++) {
			encode(map, c, (c - startCode) + startGID);
		}
	}
}

static void caryll_read_format_4(font_file_pointer start, uint32_t lengthLimit, table_cmap *map) {
	if (lengthLimit < 14) return;
	uint16_t segmentsCount = read_16u(start + 6) / 2;
	if (lengthLimit < 16 + segmentsCount * 8) return;
	for (uint16_t j = 0; j < segmentsCount; j++) {
		uint16_t endCode = read_16u(start + 14 + j * 2);
		uint16_t startCode = read_16u(start + 14 + segmentsCount * 2 + 2 + j * 2);
		int16_t idDelta = read_16u(start + 14 + segmentsCount * 4 + 2 + j * 2);
		uint32_t idRangeOffsetOffset = 14 + segmentsCount * 6 + 2 + j * 2;
		uint16_t idRangeOffset = read_16u(start + idRangeOffsetOffset);
		if (idRangeOffset == 0) {
			for (uint32_t c = startCode; c < 0xFFFF && c <= endCode; c++) {
				uint16_t gid = (c + idDelta) & 0xFFFF;
				encode(map, c, gid);
			}
		} else {
			for (uint32_t c = startCode; c < 0xFFFF && c <= endCode; c++) {
				uint32_t glyphOffset = idRangeOffset + (c - startCode) * 2 + idRangeOffsetOffset;
				if (glyphOffset + 2 > lengthLimit) continue; // ignore this encoding slot when o-o-r
				uint16_t gid = (read_16u(start + glyphOffset) + idDelta) & 0xFFFF;
				encode(map, c, gid);
			}
		}
	}
}

static void caryll_read_mapping_table(font_file_pointer start, uint32_t lengthLimit, table_cmap *map) {
	uint16_t format = read_16u(start);
	if (format == 4) {
		caryll_read_format_4(start, lengthLimit, map);
	} else if (format == 12) {
		caryll_read_format_12(start, lengthLimit, map);
	}
}

static int by_unicode(cmap_Entry *a, cmap_Entry *b) {
	return (a->unicode - b->unicode);
}

// OTFCC will not support all `cmap` mappings.
table_cmap *table_read_cmap(caryll_Packet packet) {
	// the map is a reference to a hash table
	table_cmap *map = NULL;
	FOR_TABLE('cmap', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 4) goto CMAP_CORRUPTED;

		map = malloc(sizeof(table_cmap));
		*map = NULL; // intialize to empty hashtable
		uint16_t numTables = read_16u(data + 2);
		if (length < 4 + 8 * numTables) goto CMAP_CORRUPTED;
		for (uint16_t j = 0; j < numTables; j++) {
			uint16_t platform = read_16u(data + 4 + 8 * j);
			uint16_t encoding = read_16u(data + 4 + 8 * j + 2);
			if ((platform == 0 && encoding == 3) || (platform == 0 && encoding == 4) ||
			    (platform == 3 && encoding == 1) || (platform == 3 && encoding == 10)) {
				uint32_t tableOffset = read_32u(data + 4 + 8 * j + 4);
				caryll_read_mapping_table(data + tableOffset, length - tableOffset, map);
			}
		};
		HASH_SORT(*map, by_unicode);
		return map;

	CMAP_CORRUPTED:
		fprintf(stderr, "table 'cmap' corrupted.\n");
		if (map != NULL) { free(map), map = NULL; }
	}
	return NULL;
}

void table_delete_cmap(table_cmap *table) {
	cmap_Entry *s, *tmp;
	HASH_ITER(hh, *(table), s, tmp) {
		// delete and free all cmap entries
		handle_delete(&s->glyph);
		HASH_DEL(*(table), s);
		free(s);
	}
	free(table);
}

void table_dump_cmap(table_cmap *table, json_value *root, const caryll_Options *options) {
	if (!table) return;
	if (options->verbose) fprintf(stderr, "Dumping cmap.\n");

	json_value *cmap = json_object_new(HASH_COUNT(*table));

	cmap_Entry *item;
	foreach_hash(item, *table) if (item->glyph.name) {
		sds key = sdsfromlonglong(item->unicode);
		json_object_push(cmap, key, json_string_new_length((uint32_t)sdslen(item->glyph.name), item->glyph.name));
		sdsfree(key);
	}
	json_object_push(root, "cmap", cmap);
}

table_cmap *table_parse_cmap(json_value *root, const caryll_Options *options) {
	if (root->type != json_object) return NULL;
	table_cmap hash = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "cmap", json_object))) {
		if (options->verbose) fprintf(stderr, "Parsing cmap.\n");
		for (uint32_t j = 0; j < table->u.object.length; j++) {
			sds unicodeStr = sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
			json_value *item = table->u.object.values[j].value;
			int32_t unicode = atoi(unicodeStr);
			sdsfree(unicodeStr);
			if (item->type == json_string && unicode > 0 && unicode <= 0x10FFFF) {
				sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
				cmap_Entry *item = NULL;
				HASH_FIND_INT(hash, &unicode, item);
				if (!item) {
					item = calloc(1, sizeof(cmap_Entry));
					item->unicode = unicode;
					item->glyph = handle_fromName(gname);
					HASH_ADD_INT(hash, unicode, item);
				} else {
					sdsfree(gname);
				}
			}
		}
	}
	if (hash) {
		HASH_SORT(hash, by_unicode);
		table_cmap *map = malloc(sizeof(table_cmap *));
		*map = hash;
		return map;
	}
	return NULL;
}
// writing tables
#define FLUSH_SEQUENCE_FORMAT_4                                                                                        \
	bufwrite16b(endCount, lastUnicodeEnd);                                                                             \
	bufwrite16b(startCount, lastUnicodeStart);                                                                         \
	if (isSequencial) {                                                                                                \
		bufwrite16b(idDelta, lastGIDStart - lastUnicodeStart);                                                         \
		bufwrite16b(idRangeOffset, 0);                                                                                 \
	} else {                                                                                                           \
		bufwrite16b(idDelta, 0);                                                                                       \
		bufwrite16b(idRangeOffset, lastGlyphIdArrayOffset + 1);                                                        \
	}                                                                                                                  \
	segmentsCount += 1;
caryll_buffer *table_build_cmap_format4(table_cmap *cmap) {
	caryll_buffer *buf = bufnew();
	caryll_buffer *endCount = bufnew();
	caryll_buffer *startCount = bufnew();
	caryll_buffer *idDelta = bufnew();
	caryll_buffer *idRangeOffset = bufnew();
	caryll_buffer *glyphIdArray = bufnew();

	bool started = false;
	int lastUnicodeStart = 0xFFFFFF;
	int lastUnicodeEnd = 0xFFFFFF;
	int lastGIDStart = 0xFFFFFF;
	int lastGIDEnd = 0xFFFFFF;
	size_t lastGlyphIdArrayOffset = 0;
	bool isSequencial = true;
	uint16_t segmentsCount = 0;

	cmap_Entry *item;
	foreach_hash(item, *cmap) if (item->unicode <= 0xFFFF) {
		if (!started) {
			started = true;
			lastUnicodeStart = lastUnicodeEnd = item->unicode;
			lastGIDStart = lastGIDEnd = item->glyph.index;
			isSequencial = true;
		} else {
			if (item->unicode == lastUnicodeEnd + 1 &&
			    !(item->glyph.index != lastGIDEnd + 1 && isSequencial && lastGIDEnd - lastGIDStart >= 4)) {
				if (isSequencial && !(item->glyph.index == lastGIDEnd + 1)) {
					lastGlyphIdArrayOffset = glyphIdArray->cursor;
					// oops, sequencial glyphid broken
					for (int j = lastGIDStart; j <= lastGIDEnd; j++) {
						bufwrite16b(glyphIdArray, j);
					}
				}
				lastUnicodeEnd = item->unicode;
				isSequencial = isSequencial && (item->glyph.index == lastGIDEnd + 1);
				lastGIDEnd = item->glyph.index;
				if (!isSequencial) { bufwrite16b(glyphIdArray, lastGIDEnd); }
			} else {
				// we have a segment
				FLUSH_SEQUENCE_FORMAT_4;

				lastUnicodeStart = lastUnicodeEnd = item->unicode;
				lastGIDStart = lastGIDEnd = item->glyph.index;
				isSequencial = true;
			}
		}
	}

	FLUSH_SEQUENCE_FORMAT_4;
	if (lastGIDEnd < 0xFFFF) {
		// Add a padding segment to end this subtable
		bufwrite16b(endCount, 0xFFFF);
		bufwrite16b(startCount, 0xFFFF);
		bufwrite16b(idDelta, 1);
		bufwrite16b(idRangeOffset, 0);
		segmentsCount += 1;
	}

	for (int j = 0; j < segmentsCount; j++) {
		// rewrite idRangeOffset
		uint16_t ro = read_16u((uint8_t *)idRangeOffset->data + j * 2);
		if (ro) {
			ro -= 1;
			ro += 2 * (segmentsCount - j);
			bufseek(idRangeOffset, 2 * j);
			bufwrite16b(idRangeOffset, ro);
		}
	}

	bufwrite16b(buf, 4);
	bufwrite16b(buf, 0); // fill later
	bufwrite16b(buf, 0);
	bufwrite16b(buf, segmentsCount << 1);
	uint32_t i;
	uint32_t j;
	for (j = 0, i = 1; i <= segmentsCount; ++j) {
		i <<= 1;
	}
	bufwrite16b(buf, i);
	bufwrite16b(buf, j - 1);
	bufwrite16b(buf, 2 * segmentsCount - i);
	bufwrite_buf(buf, endCount);
	bufwrite16b(buf, 0);
	bufwrite_buf(buf, startCount);
	bufwrite_buf(buf, idDelta);
	bufwrite_buf(buf, idRangeOffset);
	bufwrite_buf(buf, glyphIdArray);

	bufseek(buf, 2);
	bufwrite16b(buf, buflen(buf));

	buffree(endCount);
	buffree(startCount);
	buffree(idDelta);
	buffree(idRangeOffset);
	buffree(glyphIdArray);
	return buf;
}
caryll_buffer *table_build_cmap_format12(table_cmap *cmap) {
	caryll_buffer *buf = bufnew();
	bufwrite16b(buf, 12);
	bufwrite16b(buf, 0);
	bufwrite32b(buf, 0); // fill later
	bufwrite32b(buf, 0);
	bufwrite32b(buf, 0); // fill later

	uint32_t nGroups = 0;
	bool started = false;
	int lastUnicodeStart = 0xFFFFFF;
	int lastUnicodeEnd = 0xFFFFFF;
	int lastGIDStart = 0xFFFFFF;
	int lastGIDEnd = 0xFFFFFF;
	cmap_Entry *item;
	foreach_hash(item, *cmap) {
		if (!started) {
			started = true;
			lastUnicodeStart = lastUnicodeEnd = item->unicode;
			lastGIDStart = lastGIDEnd = item->glyph.index;
		} else if (item->unicode == lastUnicodeEnd + 1 && item->glyph.index == lastGIDEnd + 1) {
			lastUnicodeEnd = item->unicode;
			lastGIDEnd = item->glyph.index;
		} else {
			bufwrite32b(buf, lastUnicodeStart);
			bufwrite32b(buf, lastUnicodeEnd);
			bufwrite32b(buf, lastGIDStart);
			nGroups += 1;
			lastUnicodeStart = lastUnicodeEnd = item->unicode;
			lastGIDStart = lastGIDEnd = item->glyph.index;
		}
	}
	bufwrite32b(buf, lastUnicodeStart);
	bufwrite32b(buf, lastUnicodeEnd);
	bufwrite32b(buf, lastGIDStart);
	nGroups += 1;

	bufseek(buf, 4);
	bufwrite32b(buf, (uint32_t)buflen(buf));
	bufseek(buf, 12);
	bufwrite32b(buf, nGroups);
	return buf;
}
caryll_buffer *table_build_cmap(table_cmap *cmap, const caryll_Options *options) {
	caryll_buffer *buf = bufnew();
	if (!cmap || !*cmap) return buf;

	cmap_Entry *entry;
	bool hasSMP = false;
	foreach_hash(entry, *cmap) {
		if (entry->unicode > 0xFFFF) { hasSMP = true; }
	}

	bufwrite16b(buf, 0);
	uint8_t nTables = hasSMP ? 4 : 2;
	bufwrite16b(buf, nTables);
	uint32_t offset = 4 + 8 * nTables;
	size_t cp = 0;
	if (true) {
		caryll_buffer *format4 = table_build_cmap_format4(cmap);
		// Windows format 4;
		bufwrite16b(buf, 3);
		bufwrite16b(buf, 1);
		bufwrite32b(buf, offset);
		// Unicode format 4:
		bufwrite16b(buf, 0);
		bufwrite16b(buf, 3);
		bufwrite32b(buf, offset);
		cp = buf->cursor;
		bufseek(buf, offset);
		bufwrite_buf(buf, format4);
		bufseek(buf, cp);
		offset += buflen(format4);
		buffree(format4);
	}
	if (hasSMP) {
		caryll_buffer *format12 = table_build_cmap_format12(cmap);
		// Windows format 12;
		bufwrite16b(buf, 3);
		bufwrite16b(buf, 10);
		bufwrite32b(buf, offset);
		// Unicode format 12:
		bufwrite16b(buf, 0);
		bufwrite16b(buf, 4);
		bufwrite32b(buf, offset);
		cp = buf->cursor;
		bufseek(buf, offset);
		bufwrite_buf(buf, format12);
		bufseek(buf, cp);
		offset += buflen(format12);
		buffree(format12);
	}
	return buf;
}
