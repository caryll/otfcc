#include "CPAL.h"

#include "support/util.h"

// Colors are trivial.
caryll_standardType(cpal_Color, cpal_iColor);
caryll_standardVectorImpl(cpal_ColorSet, cpal_Color, cpal_iColor, cpal_iColorSet);

// Palettes are non-trivial.
static INLINE void initPalette(cpal_Palette *p) {
	cpal_iColorSet.init(&p->colorset);
	p->type = 0;
	p->label = 0xFFFF;
}
static INLINE void disposePalette(cpal_Palette *p) {
	cpal_iColorSet.dispose(&p->colorset);
}
caryll_standardType(cpal_Palette, cpal_iPalette, initPalette, disposePalette);
caryll_standardVectorImpl(cpal_PaletteSet, cpal_Palette, cpal_iPalette, cpal_iPaletteSet);

// CPAL table
static INLINE void initCPAL(table_CPAL *cpal) {
	cpal->version = 1;
	cpal_iPaletteSet.init(&cpal->palettes);
}
static INLINE void disposeCPAL(table_CPAL *cpal) {
	cpal_iPaletteSet.dispose(&cpal->palettes);
}
caryll_standardRefType(table_CPAL, iTable_CPAL, initCPAL, disposeCPAL);

const cpal_Color white = {.red = 0xFF, .green = 0xFF, .blue = 0xFF, .alpha = 0xFF, .label = 0xFFFF};

table_CPAL *otfcc_readCPAL(const otfcc_Packet packet, const otfcc_Options *options) {
	table_CPAL *t = NULL;
	FOR_TABLE('CPAL', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 2) goto FAIL;
		t = iTable_CPAL.create();
		uint16_t version = read_16u(data);
		uint32_t tableHeaderLength = (version == 0 ? 14 : 26);
		if (length < tableHeaderLength) goto FAIL;
		uint16_t numPalettesEntries = read_16u(data + 2);
		uint16_t numPalettes = read_16u(data + 4);
		uint16_t numColorRecords = read_16u(data + 6);
		uint32_t offsetFirstColorRecord = read_32u(data + 8);
		if (length < offsetFirstColorRecord + numColorRecords * 4) goto FAIL;
		if (length < tableHeaderLength + 2 * numPalettes) goto FAIL;

		// read color list
		cpal_Color *colorList;
		NEW(colorList, numColorRecords);
		for (uint16_t j = 0; j < numColorRecords; j++) {
			colorList[j] = (cpal_Color){.blue = read_8u(data + offsetFirstColorRecord + j * 4),
			                            .green = read_8u(data + offsetFirstColorRecord + j * 4 + 1),
			                            .red = read_8u(data + offsetFirstColorRecord + j * 4 + 2),
			                            .alpha = read_8u(data + offsetFirstColorRecord + j * 4 + 3),
			                            .label = 0xFFFF};
		}

		// read palettes
		for (tableid_t j = 0; j < numPalettes; j++) {
			cpal_Palette palette;
			cpal_iPalette.init(&palette);
			tableid_t paletteStartIndex = read_16u(data + 12 + j * 2);
			for (tableid_t j = 0; j < numPalettesEntries; j++) {
				if (paletteStartIndex + j < numColorRecords) {
					cpal_iColorSet.push(&palette.colorset, colorList[j + paletteStartIndex]);
				} else {
					cpal_iColorSet.push(&palette.colorset, white);
				}
			}
			cpal_iPaletteSet.push(&t->palettes, palette);
		}

		if (version > 0) {
			uint32_t offsetPaletteTypeArray = read_32u(data + 16 + 2 * numPalettes);
			if (offsetPaletteTypeArray && length >= offsetPaletteTypeArray + 4 * numPalettes) {
				for (tableid_t j = 0; j < numPalettes; j++) {
					uint32_t type = read_32u(data + j * 4 + offsetPaletteTypeArray);
					t->palettes.items[j].type = type;
				}
			}
			uint32_t offsetPaletteLabelArray = read_32u(data + 20 + 2 * numPalettes);
			if (offsetPaletteLabelArray && length >= offsetPaletteLabelArray + 2 * numPalettes) {
				for (tableid_t j = 0; j < numPalettes; j++) {
					uint16_t label = read_16u(data + j * 2 + offsetPaletteLabelArray);
					t->palettes.items[j].label = label;
				}
			}

			if (version > 0) {
				uint32_t offsetPaletteEntryLabelArray = read_32u(data + 24 + 2 * numPalettes);
				if (offsetPaletteEntryLabelArray && length >= offsetPaletteEntryLabelArray + 4 * numPalettesEntries) {
					for (tableid_t j = 0; j < numPalettesEntries; j++) {
						uint16_t label = read_16u(data + j * 2 + offsetPaletteEntryLabelArray);
						for (tableid_t k = 0; k < numPalettes; k++) {
							t->palettes.items[k].colorset.items[j].label = label;
						}
					}
				}
			}
		}

		FREE(colorList);
		return t;

	FAIL:
		iTable_CPAL.free(t);
		t = NULL;
	}
	return NULL;
}

static INLINE json_value *dumpColor(cpal_Color *color) {
	json_value *_color = json_object_new(5);
	json_object_push(_color, "red", json_integer_new(color->red));
	json_object_push(_color, "green", json_integer_new(color->green));
	json_object_push(_color, "blue", json_integer_new(color->blue));
	if (color->alpha != 0xFF) json_object_push(_color, "alpha", json_integer_new(color->alpha));
	if (color->label != 0xFFFF) json_object_push(_color, "label", json_integer_new(color->label));
	return preserialize(_color);
}

static INLINE json_value *dumpPalette(cpal_Palette *palette) {
	json_value *_palette = json_object_new(3);
	if (palette->type) { json_object_push(_palette, "type", json_integer_new(palette->type)); }
	if (palette->label != 0xFFFF) { json_object_push(_palette, "label", json_integer_new(palette->label)); }
	json_value *a = json_array_new(palette->colorset.length);
	for (tableid_t j = 0; j < palette->colorset.length; j++) {
		json_array_push(a, dumpColor(&palette->colorset.items[j]));
	}
	json_object_push(_palette, "colors", a);
	return _palette;
}

void otfcc_dumpCPAL(const table_CPAL *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("CPAL") {
		json_value *_t = json_object_new(2);
		json_object_push(_t, "version", json_integer_new(table->version));
		json_value *_a = json_array_new(table->palettes.length);
		for (tableid_t j = 0; j < table->palettes.length; j++) {
			json_array_push(_a, dumpPalette(&table->palettes.items[j]));
		}
		json_object_push(_t, "palettes", _a);
		json_object_push(root, "CPAL", _t);
	}
}
