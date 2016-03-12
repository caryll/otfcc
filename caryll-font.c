#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caryll-sfnt.h"
#include "caryll-font.h"
#include "caryll-io.h"

void caryll_read_head(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'head') {
			uint8_t *data = packet.pieces[i].data;
			uint32_t length = packet.pieces[i].length;

			if (length < 54) {
				printf("table 'head' corrupted.\n");
				font->head = NULL;
			} else {
				table_head *head = (table_head *)malloc(sizeof(table_head) * 1);
				head->version = caryll_blt32u(data);
				head->fontRevison = caryll_blt32u(data + 4);
				head->checkSumAdjustment = caryll_blt32u(data + 8);
				head->magicNumber = caryll_blt32u(data + 12);
				head->flags = caryll_blt16u(data + 16);
				head->unitsPerEm = caryll_blt16u(data + 18);
				head->created = caryll_blt64u(data + 20);
				head->modified = caryll_blt64u(data + 28);
				head->xMin = caryll_blt16u(data + 36);
				head->yMin = caryll_blt16u(data + 38);
				head->xMax = caryll_blt16u(data + 40);
				head->yMax = caryll_blt16u(data + 42);
				head->macStyle = caryll_blt16u(data + 44);
				head->lowestRecPPEM = caryll_blt16u(data + 46);
				head->fontDirectoryHint = caryll_blt16u(data + 48);
				head->indexToLocFormat = caryll_blt16u(data + 50);
				head->glyphDataFormat = caryll_blt16u(data + 52);
				font->head = head;
			}
		}
	}
}

void caryll_read_hhea(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'hhea') {
			uint8_t *data = packet.pieces[i].data;
			uint32_t length = packet.pieces[i].length;

			if (length < 36) {
				printf("table 'hhea' corrupted.\n");
				font->hhea = NULL;
			} else {
				table_hhea *hhea = (table_hhea *)malloc(sizeof(table_hhea) * 1);
				hhea->version = caryll_blt32u(data);
				hhea->ascender = caryll_blt16u(data + 4);
				hhea->descender = caryll_blt16u(data + 6);
				hhea->lineGap = caryll_blt16u(data + 8);
				hhea->advanceWithMax = caryll_blt16u(data + 10);
				hhea->minLeftSideBearing = caryll_blt16u(data + 12);
				hhea->minRightSideBearing = caryll_blt16u(data + 14);
				hhea->xMaxExtent = caryll_blt16u(data + 16);
				hhea->caretSlopeRise = caryll_blt16u(data + 18);
				hhea->caretSlopeRun = caryll_blt16u(data + 20);
				hhea->caretOffset = caryll_blt16u(data + 22);
				hhea->reserved[0] = caryll_blt16u(data + 24);
				hhea->reserved[1] = caryll_blt16u(data + 26);
				hhea->reserved[2] = caryll_blt16u(data + 28);
				hhea->reserved[3] = caryll_blt16u(data + 30);
				hhea->metricDataFormat = caryll_blt16u(data + 32);
				hhea->numberOfMetrics = caryll_blt16u(data + 34);
				font->hhea = hhea;
			}
		}
	}
}

void caryll_read_maxp(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'maxp') {
			uint8_t *data = packet.pieces[i].data;
			uint32_t length = packet.pieces[i].length;

			if (length != 32 && length != 6) {
				printf("table 'maxp' corrupted.\n");
				font->maxp = NULL;
			} else {
				table_maxp *maxp = (table_maxp *)malloc(sizeof(table_maxp) * 1);
				maxp->version = caryll_blt32u(data);
				maxp->numGlyphs = caryll_blt16u(data + 4);
				if (maxp->version == 0x00010000) { // TrueType Format 1
					maxp->maxPoints = caryll_blt16u(data + 6);
					maxp->maxContours = caryll_blt16u(data + 8);
					maxp->maxCompositePoints = caryll_blt16u(data + 10);
					maxp->maxCompositeContours = caryll_blt16u(data + 12);
					maxp->maxZones = caryll_blt16u(data + 14);
					maxp->maxTwilightPoints = caryll_blt16u(data + 16);
					maxp->maxStorage = caryll_blt16u(data + 18);
					maxp->maxFunctionDefs = caryll_blt16u(data + 20);
					maxp->maxInstructionDefs = caryll_blt16u(data + 22);
					maxp->maxStackElements = caryll_blt16u(data + 24);
					maxp->maxSizeOfInstructions = caryll_blt16u(data + 26);
					maxp->maxComponentElements = caryll_blt16u(data + 28);
					maxp->maxComponentDepth = caryll_blt16u(data + 30);
				} else { // CFF OTF Format 0.5
					maxp->maxPoints = 0;
					maxp->maxContours = 0;
					maxp->maxCompositePoints = 0;
					maxp->maxCompositeContours = 0;
					maxp->maxZones = 0;
					maxp->maxTwilightPoints = 0;
					maxp->maxStorage = 0;
					maxp->maxFunctionDefs = 0;
					maxp->maxInstructionDefs = 0;
					maxp->maxStackElements = 0;
					maxp->maxSizeOfInstructions = 0;
					maxp->maxComponentElements = 0;
					maxp->maxComponentDepth = 0;
				}
				font->maxp = maxp;
			}
		}
	}
}

void caryll_read_hmtx(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'hmtx') {
			uint8_t *data = packet.pieces[i].data;

			if (font->hhea == NULL)
				font->hmtx = NULL;
			else {
				if (font->hhea->numberOfMetrics == 0)
					font->hmtx = NULL;
				else {
					table_hmtx *hmtx = (table_hmtx *)malloc(sizeof(table_hmtx) * 1);

					uint32_t count_a = font->hhea->numberOfMetrics;
					uint32_t count_k = font->maxp->numGlyphs - font->hhea->numberOfMetrics;

					hmtx->metrics = (horizontal_metric *)malloc(sizeof(horizontal_metric) * count_a);
					hmtx->leftSideBearing = (int16_t *)malloc(sizeof(int16_t) * count_k);

					for (uint32_t ia = 0; ia < count_a; ia++) {
						hmtx->metrics[ia].advanceWidth = caryll_blt16u(data + ia * 4);
						hmtx->metrics[ia].lsb = caryll_blt16u(data + ia * 4 + 2);
					}

					for (uint32_t ik = 0; ik < count_k; ik++) {
						hmtx->leftSideBearing[ik] = caryll_blt16u(data + count_a * 4 + ik * 2);
					}

					font->hmtx = hmtx;
				}
			}
		}
	}
}

void caryll_read_post(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'post') {
			uint8_t *data = packet.pieces[i].data;

			table_post *post = (table_post *)malloc(sizeof(table_post) * 1);
			post->version = caryll_blt32u(data);
			post->italicAngle = caryll_blt32u(data + 4);
			post->underlinePosition = caryll_blt16u(data + 8);
			post->underlineThickness = caryll_blt16u(data + 10);
			post->isFixedPitch = caryll_blt32u(data + 12);
			post->minMemType42 = caryll_blt32u(data + 16);
			post->maxMemType42 = caryll_blt32u(data + 20);
			post->minMemType1 = caryll_blt32u(data + 24);
			post->maxMemType1 = caryll_blt32u(data + 28);
			font->post = post;
		}
	}
}

void caryll_read_hdmx(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'hdmx') {
			uint8_t *data = packet.pieces[i].data;

			table_hdmx *hdmx = (table_hdmx *)malloc(sizeof(table_hdmx) * 1);
			hdmx->version = caryll_blt16u(data);
			hdmx->numRecords = caryll_blt16u(data + 2);
			hdmx->sizeDeviceRecord = caryll_blt32u(data + 4);
			hdmx->records = (device_record *)malloc(sizeof(device_record) * hdmx->numRecords);

			for (uint32_t i = 0; i < hdmx->numRecords; i++) {
				hdmx->records[i].pixelSize = *(data + 8 + i * (2 + font->maxp->numGlyphs));
				hdmx->records[i].maxWidth = *(data + 8 + i * (2 + font->maxp->numGlyphs) + 1);
				hdmx->records[i].widths = (uint8_t *)malloc(sizeof(uint8_t) * font->maxp->numGlyphs);
				memcpy(hdmx->records[i].widths, data + 8 + i * (2 + font->maxp->numGlyphs) + 2, font->maxp->numGlyphs);
			}

			font->hdmx = hdmx;
		}
	}
}

void caryll_read_LTSH(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'LTSH') {
			uint8_t *data = packet.pieces[i].data;

			table_LTSH *LTSH = (table_LTSH *)malloc(sizeof(table_LTSH) * 1);
			LTSH->version = caryll_blt16u(data);
			LTSH->numGlyphs = caryll_blt16u(data + 2);
			LTSH->yPels = (uint8_t *)malloc(sizeof(uint8_t) * LTSH->numGlyphs);
			memcpy(LTSH->yPels, data + 4, LTSH->numGlyphs);

			font->LTSH = LTSH;
		}
	}
}

void caryll_read_PCLT(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'PCLT') {
			uint8_t *data = packet.pieces[i].data;

			table_PCLT *PCLT = (table_PCLT *)malloc(sizeof(table_PCLT) * 1);
			PCLT->version = caryll_blt32u(data);
			PCLT->FontNumber = caryll_blt32u(data + 4);
			PCLT->Pitch = caryll_blt16u(data + 8);
			PCLT->xHeight = caryll_blt16u(data + 10);
			PCLT->Style = caryll_blt16u(data + 12);
			PCLT->TypeFamily = caryll_blt16u(data + 14);
			PCLT->CapHeight = caryll_blt16u(data + 16);
			PCLT->SymbolSet = caryll_blt16u(data + 18);
			memcpy(PCLT->Typeface, data + 20, 16);
			memcpy(PCLT->CharacterComplement, data + 36, 8);
			memcpy(PCLT->FileName, data + 44, 6);
			PCLT->StrokeWeight = *(data + 50);
			PCLT->WidthType = *(data + 51);
			PCLT->SerifStyle = *(data + 52);
			PCLT->pad = 0;

			font->PCLT = PCLT;
		}
	}
}

void caryll_read_vhea(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'vhea') {
			uint8_t *data = packet.pieces[i].data;

			table_vhea *vhea = (table_vhea *)malloc(sizeof(table_vhea) * 1);
			vhea->version = caryll_blt32u(data);
			vhea->ascent = caryll_blt16u(data + 4);
			vhea->descent = caryll_blt16u(data + 6);
			vhea->advanceHeightMax = caryll_blt16u(data + 8);
			vhea->minTop = caryll_blt16u(data + 10);
			vhea->minBottom = caryll_blt16u(data + 12);
			vhea->yMaxExtent = caryll_blt16u(data + 14);
			vhea->caretSlopeRise = caryll_blt16u(data + 16);
			vhea->caretSlopeRun = caryll_blt16u(data + 18);
			vhea->caretOffset = caryll_blt16u(data + 20);
			vhea->dummy[0] = *(data + 22);
			vhea->dummy[1] = *(data + 23);
			vhea->dummy[2] = *(data + 24);
			vhea->dummy[3] = *(data + 24);
			vhea->metricDataFormat = caryll_blt16u(data + 26);
			vhea->numOf = caryll_blt16u(data + 28);

			font->vhea = vhea;
		}
	}
}

caryll_font *caryll_font_open(caryll_sfnt *sfnt, uint32_t index) {
	if (sfnt->count - 1 < index)
		return NULL;
	else {
		caryll_font *font = (caryll_font *)malloc(sizeof(caryll_font) * 1);
		caryll_packet packet = sfnt->packets[index];

		font->head = NULL;
		font->hhea = NULL;
		font->maxp = NULL;
		font->hmtx = NULL;
		font->post = NULL;
		font->hdmx = NULL;

		caryll_read_head(font, packet);
		caryll_read_hhea(font, packet);
		caryll_read_maxp(font, packet);
		caryll_read_hmtx(font, packet);
		caryll_read_post(font, packet);
		caryll_read_hdmx(font, packet);

		return font;
	}
}

void caryll_font_close(caryll_font *font) {
	if (font->head != NULL) free(font->head);
	if (font->hhea != NULL) free(font->hhea);
	if (font->maxp != NULL) free(font->maxp);
	if (font->hmtx != NULL) {
		if (font->hmtx->metrics != NULL) free(font->hmtx->metrics);
		if (font->hmtx->leftSideBearing != NULL) free(font->hmtx->leftSideBearing);
		free(font->hmtx);
	}
	if (font->post != NULL) free(font->post);
	if (font->hdmx != NULL) {
		if (font->hdmx->records != NULL) {
			for (uint32_t i = 0; i < font->hdmx->numRecords; i++) {
				if (font->hdmx->records[i].widths != NULL)
					;
				free(font->hdmx->records[i].widths);
			}
			free(font->hdmx->records);
		}
		free(font->hdmx);
	}
	if (font != NULL) free(font);
}
