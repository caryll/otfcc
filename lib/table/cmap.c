#include "cmap.h"

#include "support/util.h"
#include "bk/bkgraph.h"

// PART I, type definition

static INLINE void initCmap(table_cmap *cmap) {
	cmap->unicodes = NULL;
	cmap->uvs = NULL;
}
static INLINE void disposeCmap(table_cmap *cmap) {
	{ // Unicode
		cmap_Entry *s, *tmp;
		HASH_ITER(hh, cmap->unicodes, s, tmp) {
			// delete and free all cmap entries
			Handle.dispose(&s->glyph);
			HASH_DEL(cmap->unicodes, s);
			FREE(s);
		}
	}
	{ // UVS
		cmap_UVS_Entry *s, *tmp;
		HASH_ITER(hh, cmap->uvs, s, tmp) {
			// delete and free all cmap entries
			Handle.dispose(&s->glyph);
			HASH_DEL(cmap->uvs, s);
			FREE(s);
		}
	}
}
caryll_standardRefTypeFn(table_cmap, initCmap, disposeCmap);

bool otfcc_encodeCmapByIndex(table_cmap *cmap, int c, uint16_t gid) {
	cmap_Entry *s;
	HASH_FIND_INT(cmap->unicodes, &c, s);
	if (s == NULL) {
		NEW(s);
		s->glyph = Handle.fromIndex(gid);
		s->unicode = c;
		HASH_ADD_INT(cmap->unicodes, unicode, s);
		return true;
	} else {
		return false;
	}
}
bool otfcc_encodeCmapByName(table_cmap *cmap, int c, sds name) {
	cmap_Entry *s;
	HASH_FIND_INT(cmap->unicodes, &c, s);
	if (s == NULL) {
		NEW(s);
		s->glyph = Handle.fromName(name);
		s->unicode = c;
		HASH_ADD_INT(cmap->unicodes, unicode, s);
		return true;
	} else {
		return false;
	}
}
bool otfcc_unmapCmap(table_cmap *cmap, int c) {
	cmap_Entry *s;
	HASH_FIND_INT(cmap->unicodes, &c, s);
	if (s) {
		Handle.dispose(&s->glyph);
		HASH_DEL(cmap->unicodes, s);
		FREE(s);
		return true;
	} else {
		return false;
	}
}

otfcc_GlyphHandle *otfcc_cmapLookup(table_cmap *cmap, int c) {
	cmap_Entry *s;
	HASH_FIND_INT(cmap->unicodes, &c, s);
	if (s) {
		return &(s->glyph);
	} else {
		return NULL;
	}
}

bool otfcc_encodeCmapUVSByIndex(table_cmap *cmap, cmap_UVS_key c, uint16_t gid) {
	cmap_UVS_Entry *s;
	HASH_FIND(hh, cmap->uvs, &c, sizeof(cmap_UVS_key), s);
	if (s == NULL) {
		NEW(s);
		s->glyph = Handle.fromIndex(gid);
		s->key = c;
		HASH_ADD(hh, cmap->uvs, key, sizeof(cmap_UVS_key), s);
		return true;
	} else {
		return false;
	}
}
bool otfcc_encodeCmapUVSByName(table_cmap *cmap, cmap_UVS_key c, sds name) {
	cmap_UVS_Entry *s;
	HASH_FIND(hh, cmap->uvs, &c, sizeof(cmap_UVS_key), s);
	if (s == NULL) {
		NEW(s);
		s->glyph = Handle.fromName(name);
		s->key = c;
		HASH_ADD(hh, cmap->uvs, key, sizeof(cmap_UVS_key), s);
		return true;
	} else {
		return false;
	}
}
bool otfcc_unmapCmapUVS(table_cmap *cmap, cmap_UVS_key c) {
	cmap_Entry *s;
	HASH_FIND(hh, cmap->uvs, &c, sizeof(cmap_UVS_key), s);
	if (s) {
		Handle.dispose(&s->glyph);
		HASH_DEL(cmap->uvs, s);
		FREE(s);
		return true;
	} else {
		return false;
	}
}

otfcc_GlyphHandle *otfcc_cmapLookupUVS(table_cmap *cmap, cmap_UVS_key c) {
	cmap_Entry *s;
	HASH_FIND(hh, cmap->uvs, &c, sizeof(cmap_UVS_key), s);
	if (s) {
		return &(s->glyph);
	} else {
		return NULL;
	}
}

caryll_ElementInterfaceOf(table_cmap) table_iCmap = {caryll_standardRefTypeMethods(table_cmap),
                                                     .encodeByIndex = otfcc_encodeCmapByIndex,
                                                     .encodeByName = otfcc_encodeCmapByName,
                                                     .unmap = otfcc_unmapCmap,
                                                     .lookup = otfcc_cmapLookup,
                                                     .encodeUVSByIndex = otfcc_encodeCmapUVSByIndex,
                                                     .encodeUVSByName = otfcc_encodeCmapUVSByName,
                                                     .unmapUVS = otfcc_unmapCmapUVS,
                                                     .lookupUVS = otfcc_cmapLookupUVS};

// PART II, reading and writing

static void readFormat12(font_file_pointer start, uint32_t lengthLimit, table_cmap *cmap) {
	if (lengthLimit < 16) return;
	uint32_t nGroups = read_32u(start + 12);
	if (lengthLimit < 16 + 12 * nGroups) return;
	for (uint32_t j = 0; j < nGroups; j++) {
		uint32_t startCode = read_32u(start + 16 + 12 * j);
		uint32_t endCode = read_32u(start + 16 + 12 * j + 4);
		uint32_t startGID = read_32u(start + 16 + 12 * j + 8);
		for (uint32_t c = startCode; c <= endCode; c++) {
			otfcc_encodeCmapByIndex(cmap, c, (c - startCode) + startGID);
		}
	}
}

static void readFormat4(font_file_pointer start, uint32_t lengthLimit, table_cmap *cmap) {
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
				otfcc_encodeCmapByIndex(cmap, c, gid);
			}
		} else {
			for (uint32_t c = startCode; c < 0xFFFF && c <= endCode; c++) {
				uint32_t glyphOffset = idRangeOffset + (c - startCode) * 2 + idRangeOffsetOffset;
				if (glyphOffset + 2 > lengthLimit) continue; // ignore this encoding slot when o-o-r
				uint16_t gid = (read_16u(start + glyphOffset) + idDelta) & 0xFFFF;
				otfcc_encodeCmapByIndex(cmap, c, gid);
			}
		}
	}
}
static void readUVSDefault(font_file_pointer start, uint32_t lengthLimit, unicode_t selector,
                           table_cmap *cmap) {
	if (lengthLimit < 4) return;
	uint32_t numUnicodeValueRanges = read_32u(start);
	if (lengthLimit < 4 + 4 * numUnicodeValueRanges) return;
	for (uint32_t j = 0; j < numUnicodeValueRanges; j++) {
		font_file_pointer vsr = start + 4 + 4 * j;
		unicode_t startUnicodeValue = read_24u(vsr);
		uint8_t additionalCount = read_8u(vsr + 3);
		for (unicode_t u = startUnicodeValue; u <= startUnicodeValue + additionalCount; u++) {
			otfcc_GlyphHandle *g = table_iCmap.lookup(cmap, (int)u);
			if (!g) continue;
			table_iCmap.encodeUVSByIndex(cmap, (cmap_UVS_key){.unicode = u, .selector = selector},
			                             g->index);
		}
	}
}
static void readUVSNonDefault(font_file_pointer start, uint32_t lengthLimit, unicode_t selector,
                              table_cmap *cmap) {
	if (lengthLimit < 4) return;
	uint32_t numUVSMappings = read_32u(start);
	if (lengthLimit < 4 + 5 * numUVSMappings) return;
	for (uint32_t j = 0; j < numUVSMappings; j++) {
		font_file_pointer vsr = start + 4 + 5 * j;
		unicode_t unicodeValue = read_24u(vsr);
		glyphid_t glyphID = read_16u(vsr + 3);
		table_iCmap.encodeUVSByIndex(
		    cmap, (cmap_UVS_key){.unicode = unicodeValue, .selector = selector}, glyphID);
	}
}
static void readFormat14(font_file_pointer start, uint32_t lengthLimit, table_cmap *cmap) {
	if (lengthLimit < 10) return;
	uint32_t nGroups = read_32u(start + 6);
	if (lengthLimit < 11 + 11 * nGroups) return;
	fprintf(stderr, "UVS: %d selectors\n", nGroups);
	for (uint32_t j = 0; j < nGroups; j++) {
		font_file_pointer vsr = start + 10 + 11 * j;
		unicode_t selector = read_24u(vsr);
		uint32_t defaultUVSOffset = read_32u(vsr + 3);
		uint32_t nonDefaultUVSOffset = read_32u(vsr + 7);
		fprintf(stderr, "Selector = %08x, DOFS = %08x, NDOFS = %08x\n", selector, defaultUVSOffset,
		        nonDefaultUVSOffset);

		if (defaultUVSOffset) {
			readUVSDefault(start + defaultUVSOffset, lengthLimit - defaultUVSOffset, selector,
			               cmap);
		}
		if (nonDefaultUVSOffset) {
			readUVSNonDefault(start + nonDefaultUVSOffset, lengthLimit - nonDefaultUVSOffset,
			                  selector, cmap);
		}
	}
}

static void readCmapMappingTable(font_file_pointer start, uint32_t lengthLimit, table_cmap *cmap) {
	uint16_t format = read_16u(start);
	if (format == 4) {
		readFormat4(start, lengthLimit, cmap);
	} else if (format == 12) {
		readFormat12(start, lengthLimit, cmap);
	}
}

static void readCmapMappingTableUVS(font_file_pointer start, uint32_t lengthLimit,
                                    table_cmap *cmap) {
	uint16_t format = read_16u(start);
	if (format == 14) { readFormat14(start, lengthLimit, cmap); }
}

static int by_unicode(cmap_Entry *a, cmap_Entry *b) {
	return (a->unicode - b->unicode);
}

static INLINE bool isValidCmapEncoding(uint16_t platform, uint16_t encoding) {
	return (platform == 0 && encoding == 3) || (platform == 0 && encoding == 4) ||
	       (platform == 0 && encoding == 5) || (platform == 3 && encoding == 1) ||
	       (platform == 3 && encoding == 10);
}

// OTFCC will not support all `cmap` mappings.
table_cmap *otfcc_readCmap(const otfcc_Packet packet, const otfcc_Options *options) {
	// the map is a reference to a hash table
	table_cmap *cmap = NULL;
	FOR_TABLE('cmap', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 4) goto CMAP_CORRUPTED;

		cmap = table_iCmap.create(); // intialize to empty hashtable
		uint16_t numTables = read_16u(data + 2);
		if (length < 4 + 8 * numTables) goto CMAP_CORRUPTED;

		// step 1 : read format 4 and 12
		for (uint16_t j = 0; j < numTables; j++) {
			uint16_t platform = read_16u(data + 4 + 8 * j);
			uint16_t encoding = read_16u(data + 4 + 8 * j + 2);
			if (!isValidCmapEncoding(platform, encoding)) continue;

			uint32_t tableOffset = read_32u(data + 4 + 8 * j + 4);
			readCmapMappingTable(data + tableOffset, length - tableOffset, cmap);
		};
		HASH_SORT(cmap->unicodes, by_unicode);

		// step2 : read format 14
		for (uint16_t j = 0; j < numTables; j++) {
			uint16_t platform = read_16u(data + 4 + 8 * j);
			uint16_t encoding = read_16u(data + 4 + 8 * j + 2);
			if (!isValidCmapEncoding(platform, encoding)) continue;

			uint32_t tableOffset = read_32u(data + 4 + 8 * j + 4);
			readCmapMappingTableUVS(data + tableOffset, length - tableOffset, cmap);
		};
		return cmap;

	CMAP_CORRUPTED:
		logWarning("table 'cmap' corrupted.\n");
		if (cmap != NULL) { FREE(cmap), cmap = NULL; }
	}
	return NULL;
}

void otfcc_dumpCmap(const table_cmap *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("cmap") {
		if (table->unicodes) {
			json_value *cmap = json_object_new(HASH_COUNT(table->unicodes));
			cmap_Entry *item;
			foreach_hash(item, table->unicodes) if (item->glyph.name) {
				sds key;
				if (options->decimal_cmap) {
					key = sdsfromlonglong(item->unicode);
				} else {
					key = sdscatprintf(sdsempty(), "U+%04X", item->unicode);
				}
				json_object_push(
				    cmap, key,
				    json_string_new_length((uint32_t)sdslen(item->glyph.name), item->glyph.name));
				sdsfree(key);
			}
			json_object_push(root, "cmap", cmap);
		}
		if (table->uvs) {
			json_value *uvs = json_object_new(HASH_COUNT(table->uvs));
			cmap_UVS_Entry *item;
			foreach_hash(item, table->uvs) if (item->glyph.name) {
				sds key;
				if (options->decimal_cmap) {
					key = sdscatprintf(sdsempty(), "%d %d", item->key.unicode, item->key.selector);
				} else {
					key = sdscatprintf(sdsempty(), "U+%04X U+%04X", item->key.unicode,
					                   item->key.selector);
				}
				json_object_push(
				    uvs, key,
				    json_string_new_length((uint32_t)sdslen(item->glyph.name), item->glyph.name));
				sdsfree(key);
			}
			json_object_push(root, "cmap_uvs", uvs);
		}
	}
}

static void parseCmapUnicodes(table_cmap *cmap, const json_value *table,
                              const otfcc_Options *options) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds unicodeStr =
		    sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		json_value *item = table->u.object.values[j].value;
		int32_t unicode;
		if (sdslen(unicodeStr) > 2 && unicodeStr[0] == 'U' && unicodeStr[1] == '+') {
			unicode = strtol(unicodeStr + 2, NULL, 16);
		} else {
			unicode = atoi(unicodeStr);
		}
		sdsfree(unicodeStr);
		if (item->type == json_string && unicode > 0 && unicode <= 0x10FFFF) {
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			if (!otfcc_encodeCmapByName(cmap, unicode, gname)) {
				glyph_handle *currentMap = otfcc_cmapLookup(cmap, unicode);
				logWarning("U+%04X is already mapped to %s. Assignment to %s is ignored.", unicode,
				           currentMap->name, gname);
			}
		}
	}
}

table_cmap *otfcc_parseCmap(const json_value *root, const otfcc_Options *options) {
	if (root->type != json_object) return NULL;
	table_cmap *cmap = table_iCmap.create();
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "cmap", json_object))) {
		loggedStep("cmap") {
			parseCmapUnicodes(cmap, table, options);
		}
	}
	HASH_SORT(cmap->unicodes, by_unicode);
	return cmap;
}
// writing tables
#define FLUSH_SEQUENCE_FORMAT_4                                                                    \
	bufwrite16b(endCount, lastUnicodeEnd);                                                         \
	bufwrite16b(startCount, lastUnicodeStart);                                                     \
	if (isSequencial) {                                                                            \
		bufwrite16b(idDelta, lastGIDStart - lastUnicodeStart);                                     \
		bufwrite16b(idRangeOffset, 0);                                                             \
	} else {                                                                                       \
		bufwrite16b(idDelta, 0);                                                                   \
		bufwrite16b(idRangeOffset, lastGlyphIdArrayOffset + 1);                                    \
	}                                                                                              \
	segmentsCount += 1;

caryll_Buffer *otfcc_buildCmap_format4(const table_cmap *cmap) {
	caryll_Buffer *buf = bufnew();
	caryll_Buffer *endCount = bufnew();
	caryll_Buffer *startCount = bufnew();
	caryll_Buffer *idDelta = bufnew();
	caryll_Buffer *idRangeOffset = bufnew();
	caryll_Buffer *glyphIdArray = bufnew();

	bool started = false;
	int lastUnicodeStart = 0xFFFFFF;
	int lastUnicodeEnd = 0xFFFFFF;
	int lastGIDStart = 0xFFFFFF;
	int lastGIDEnd = 0xFFFFFF;
	size_t lastGlyphIdArrayOffset = 0;
	bool isSequencial = true;
	uint16_t segmentsCount = 0;

	cmap_Entry *item;
	foreach_hash(item, cmap->unicodes) if (item->unicode <= 0xFFFF) {
		if (!started) {
			started = true;
			lastUnicodeStart = lastUnicodeEnd = item->unicode;
			lastGIDStart = lastGIDEnd = item->glyph.index;
			isSequencial = true;
		} else {
			if (item->unicode == lastUnicodeEnd + 1 &&
			    !(item->glyph.index != lastGIDEnd + 1 && isSequencial &&
			      lastGIDEnd - lastGIDStart >= 4)) {
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
caryll_Buffer *otfcc_buildCmap_format12(const table_cmap *cmap) {
	caryll_Buffer *buf = bufnew();
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
	foreach_hash(item, cmap->unicodes) {
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
caryll_Buffer *otfcc_buildCmap(const table_cmap *cmap, const otfcc_Options *options) {
	if (!cmap || !cmap->unicodes) return bufnew();

	cmap_Entry *entry;
	bool hasSMP = false;
	foreach_hash(entry, cmap->unicodes) {
		if (entry->unicode > 0xFFFF) { hasSMP = true; }
	}
	uint8_t nTables = hasSMP ? 4 : 2;

	caryll_Buffer *format4;
	if (!hasSMP || !options->stub_cmap4) {
		format4 = otfcc_buildCmap_format4(cmap);
	} else {
		// Write a dummy
		format4 = bufnew();
		bufwrite16b(format4, 4);      // format
		bufwrite16b(format4, 32);     // length
		bufwrite16b(format4, 0);      // language
		bufwrite16b(format4, 4);      // segCountX2
		bufwrite16b(format4, 4);      // searchRange
		bufwrite16b(format4, 1);      // entrySelector
		bufwrite16b(format4, 0);      // rangeShift
		bufwrite16b(format4, 0);      // endCount
		bufwrite16b(format4, 0xFFFF); // endCount
		bufwrite16b(format4, 0);      // endCount
		bufwrite16b(format4, 0);      // startCount
		bufwrite16b(format4, 0xFFFF); // startCount
		bufwrite16b(format4, 0);      // idDelta
		bufwrite16b(format4, 1);      // idDelta
		bufwrite16b(format4, 0);      // idRangeOffset
		bufwrite16b(format4, 0);      // idRangeOffset
	}

	caryll_Buffer *format12 = otfcc_buildCmap_format12(cmap);
	bk_Block *root = bk_new_Block(b16, 0,       // version
	                              b16, nTables, // nTables
	                              bkover);
	bk_push(root, b16, 0,                            // unicode
	        b16, 3,                                  // BMP
	        p32, bk_newBlockFromBufferCopy(format4), // table
	        bkover);
	if (hasSMP) {
		bk_push(root, b16, 0,                             // unicode
		        b16, 4,                                   // full
		        p32, bk_newBlockFromBufferCopy(format12), // table
		        bkover);
	}
	bk_push(root, b16, 3,                            // Windows
	        b16, 1,                                  // Unicode BMP
	        p32, bk_newBlockFromBufferCopy(format4), // table
	        bkover);
	if (hasSMP) {
		bk_push(root, b16, 3,                             // Windows
		        b16, 10,                                  // Unicode Full
		        p32, bk_newBlockFromBufferCopy(format12), // table
		        bkover);
	}

	buffree(format4);
	buffree(format12);
	return bk_build_Block(root);
}
