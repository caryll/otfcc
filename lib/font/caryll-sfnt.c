#include "support/util.h"
#include "otfcc/sfnt.h"

static void caryll_read_packets(caryll_SplineFontContainer *font, FILE *file) {
	for (uint32_t count = 0; count < font->count; count++) {
		(void)fseek(file, font->offsets[count], SEEK_SET);

		font->packets[count].sfnt_version = caryll_get32u(file);
		font->packets[count].numTables = caryll_get16u(file);
		font->packets[count].searchRange = caryll_get16u(file);
		font->packets[count].entrySelector = caryll_get16u(file);
		font->packets[count].rangeShift = caryll_get16u(file);
		NEW(font->packets[count].pieces, font->packets[count].numTables);

		for (uint32_t i = 0; i < font->packets[count].numTables; i++) {
			font->packets[count].pieces[i].tag = caryll_get32u(file);
			font->packets[count].pieces[i].checkSum = caryll_get32u(file);
			font->packets[count].pieces[i].offset = caryll_get32u(file);
			font->packets[count].pieces[i].length = caryll_get32u(file);
			NEW(font->packets[count].pieces[i].data, font->packets[count].pieces[i].length);
		}

		for (uint32_t i = 0; i < font->packets[0].numTables; i++) {
			(void)fseek(file, font->packets[count].pieces[i].offset, SEEK_SET);
			(void)fread(font->packets[count].pieces[i].data, font->packets[count].pieces[i].length, 1, file);
		}
	}
}

caryll_SplineFontContainer *caryll_read_SFNT(FILE *file) {
	if (!file) return NULL;
	caryll_SplineFontContainer *font;
	NEW(font);

	font->type = caryll_get32u(file);

	switch (font->type) {
		case 'OTTO':
		case 0x00010000:
		case 'true':
		case 'typ1':
			font->count = 1;
			NEW(font->offsets, font->count);
			NEW(font->packets, font->count);
			font->offsets[0] = 0;
			caryll_read_packets(font, file);
			break;

		case 'ttcf':
			(void)caryll_get32u(file);
			font->count = caryll_get32u(file);
			NEW(font->offsets, font->count);
			NEW(font->packets, font->count);

			for (uint32_t i = 0; i < font->count; i++) {
				font->offsets[i] = caryll_get32u(file);
			}

			caryll_read_packets(font, file);
			break;

		default:
			font->count = 0;
			font->offsets = NULL;
			font->packets = NULL;
			break;
	}

	fclose(file);

	return font;
}

void caryll_delete_SFNT(caryll_SplineFontContainer *font) {
	if (font->count > 0) {
		for (uint32_t count = 0; count < font->count; count++) {
			for (int i = 0; i < font->packets[count].numTables; i++) {
				FREE(font->packets[count].pieces[i].data);
			}
			FREE(font->packets[count].pieces);
		}
	}
	FREE(font);
}
