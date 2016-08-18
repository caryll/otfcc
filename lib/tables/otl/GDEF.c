#include "GDEF.h"

void caryll_delete_GDEF(table_GDEF *gdef) {
	if (gdef) {
		if (gdef->glyphClassDef) caryll_delete_classdef(gdef->glyphClassDef);
		free(gdef);
	}
}

table_GDEF *caryll_new_GDEF() {
	table_GDEF *gdef;
	NEW(gdef);
	gdef->glyphClassDef = NULL;
	gdef->ligCarets = NULL;
	gdef->markAttachClassDef = NULL;
	return gdef;
}

static caret_value readCaretValue(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	caret_value v;
	v.format = 0;
	v.coordiante = 0;
	v.pointIndex = 0xFFFF;
	checkLength(offset + 4);

	v.format = read_16u(data + offset);
	if (v.format == 2) { // attach to glyph point
		v.pointIndex = read_16u(data + offset + 2);
	} else {
		v.coordiante = read_16u(data + offset + 2);
	}
FAIL:
	return v;
}
static caret_value_record readLigCaretRecord(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	checkLength(offset + 2);
	caret_value_record g;
	g.values = NULL;
	g.caretCount = read_16u(data + offset);
	checkLength(offset + 2 + g.caretCount * 2);
	NEW_N(g.values, g.caretCount);

	for (uint16_t j = 0; j < g.caretCount; j++) {
		g.values[j] = readCaretValue(data, tableLength, offset + read_16u(data + offset + 2 + j * 2));
	}
	return g;
FAIL:
	g.caretCount = 0;
	return g;
}

table_GDEF *caryll_read_GDEF(caryll_packet packet) {
	table_GDEF *gdef = NULL;
	FOR_TABLE('GDEF', table) {
		font_file_pointer data = table.data;
		uint32_t tableLength = table.length;
		checkLength(12);
		gdef = caryll_new_GDEF();
		uint16_t classdefOffset = read_16u(data + 4);
		if (classdefOffset) { gdef->glyphClassDef = caryll_read_classdef(data, tableLength, classdefOffset); }
		uint16_t ligCaretOffset = read_16u(data + 8);
		if (ligCaretOffset) {
			checkLength(ligCaretOffset + 4);
			NEW(gdef->ligCarets);
			gdef->ligCarets->carets = NULL;

			otl_coverage *cov =
			    caryll_read_coverage(data, tableLength, ligCaretOffset + read_16u(data + ligCaretOffset));
			if (!cov || cov->numGlyphs != read_16u(data + ligCaretOffset + 2)) goto FAIL;
			checkLength(ligCaretOffset + 4 + cov->numGlyphs * 2);
			if (cov->numGlyphs) {
				gdef->ligCarets->coverage = cov;
				NEW_N(gdef->ligCarets->carets, cov->numGlyphs);
				for (uint16_t j = 0; j < cov->numGlyphs; j++) {
					gdef->ligCarets->carets[j] = readLigCaretRecord(
					    data, tableLength, ligCaretOffset + read_16u(data + ligCaretOffset + 4 + j * 2));
				}
			} else {
				caryll_delete_coverage(cov);
				FREE(gdef->ligCarets);
			}
		}
		uint16_t markAttachDefOffset = read_16u(data + 10);
		if (markAttachDefOffset) {
			gdef->markAttachClassDef = caryll_read_classdef(data, tableLength, markAttachDefOffset);
		}
		return gdef;

	FAIL:
		DELETE(caryll_delete_GDEF, gdef);
	}
	return gdef;
}

void caryll_GDEF_to_json(table_GDEF *gdef, json_value *root, const caryll_options *options) {
	if (!gdef) return;
	if (options->verbose) fprintf(stderr, "Dumping GDEF.\n");

	json_value *_gdef = json_object_new(4);
	if (gdef->glyphClassDef) { json_object_push(_gdef, "glyphClassDef", caryll_classdef_to_json(gdef->glyphClassDef)); }
	if (gdef->markAttachClassDef) {
		json_object_push(_gdef, "markAttachClassDef", caryll_classdef_to_json(gdef->markAttachClassDef));
	}
	if (gdef->ligCarets && gdef->ligCarets->coverage && gdef->ligCarets->coverage->numGlyphs) {
		json_value *_carets = json_object_new(gdef->ligCarets->coverage->numGlyphs);
		for (uint16_t j = 0; j < gdef->ligCarets->coverage->numGlyphs; j++) {
			sds name = gdef->ligCarets->coverage->glyphs[j].name;
			json_value *_record = json_array_new(gdef->ligCarets->carets[j].caretCount);

			for (uint16_t k = 0; k < gdef->ligCarets->carets[j].caretCount; k++) {
				json_value *_cv = json_object_new(1);
				if (gdef->ligCarets->carets[j].values[k].format == 2) {
					json_object_push(_cv, "atPoint", json_integer_new(gdef->ligCarets->carets[j].values[k].pointIndex));
				} else {
					json_object_push(_cv, "at", json_integer_new(gdef->ligCarets->carets[j].values[k].coordiante));
				}
				json_array_push(_record, _cv);
			}
			json_object_push(_carets, name, preserialize(_record));
		}
		json_object_push(_gdef, "ligCarets", _carets);
	}
	json_object_push(root, "GDEF", _gdef);
}

static lig_caret_table *ligCaretFromJson(json_value *_carets) {
	if (!_carets || _carets->type != json_object) return NULL;
	lig_caret_table *lc;
	NEW(lc);
	NEW(lc->coverage);
	lc->coverage->numGlyphs = _carets->u.object.length;
	NEW_N(lc->coverage->glyphs, lc->coverage->numGlyphs);
	NEW_N(lc->carets, lc->coverage->numGlyphs);

	uint16_t jj = 0;
	for (uint16_t j = 0; j < lc->coverage->numGlyphs; j++) {
		json_value *a = _carets->u.object.values[j].value;
		if (!a || a->type != json_array) continue;
		lc->coverage->glyphs[jj] =
		    handle_from_name(sdsnewlen(_carets->u.object.values[j].name, _carets->u.object.values[j].name_length));
		lc->carets[jj].caretCount = a->u.array.length;
		NEW_N(lc->carets[jj].values, lc->carets[jj].caretCount);
		for (uint16_t k = 0; k < lc->carets[jj].caretCount; k++) {
			lc->carets[jj].values[k].format = 1;
			lc->carets[jj].values[k].coordiante = 0;
			lc->carets[jj].values[k].pointIndex = 0xFFFF;
			json_value *_caret = a->u.array.values[k];
			if (_caret && _caret->type == json_object) {
				if (json_obj_get_type(_caret, "atPoint", json_integer)) {
					lc->carets[jj].values[k].format = 2;
					lc->carets[jj].values[k].pointIndex = json_obj_getint(_caret, "atPoint");
				} else {
					lc->carets[jj].values[k].coordiante = json_obj_getnum(_caret, "at");
				}
			}
		}
		jj++;
	}
	lc->coverage->numGlyphs = jj;
	return lc;
}

table_GDEF *caryll_GDEF_from_json(json_value *root, const caryll_options *options) {
	table_GDEF *gdef = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "GDEF", json_object))) {
		if (options->verbose) fprintf(stderr, "Parsing GDEF.\n");
		gdef = caryll_new_GDEF();
		gdef->glyphClassDef = caryll_classdef_from_json(json_obj_get(table, "glyphClassDef"));
		gdef->markAttachClassDef = caryll_classdef_from_json(json_obj_get(table, "markAttachClassDef"));
		gdef->ligCarets = ligCaretFromJson(json_obj_get(table, "ligCarets"));
	}
	return gdef;
}

static caryll_bkblock *writeLigCaretRec(caret_value_record *cr) {
	caryll_bkblock *bcr = new_bkblock(b16, cr->caretCount, // CaretCount
	                                  bkover);
	for (uint16_t j = 0; j < cr->caretCount; j++) {
		bkblock_push(bcr, p16,
		             new_bkblock(b16, cr->values[j].format, // format
		                         b16, cr->values[j].format == 2 ? cr->values[j].pointIndex
		                                                        : cr->values[j].coordiante, // pointindex / coordinate
		                         bkover),                                                   // CaretValue
		             bkover);
	}
	return bcr;
}

static caryll_bkblock *writeLigCarets(lig_caret_table *lc) {
	caryll_bkblock *lct = new_bkblock(p16, new_bkblock_from_buffer(caryll_write_coverage(lc->coverage)), // Coverage
	                                  b16, lc->coverage->numGlyphs, // LigGlyphCount
	                                  bkover);
	for (uint16_t j = 0; j < lc->coverage->numGlyphs; j++) {
		bkblock_push(lct, p16, writeLigCaretRec(&(lc->carets[j])), bkover);
	}
	return lct;
}

caryll_buffer *caryll_write_GDEF(table_GDEF *gdef, const caryll_options *options) {
	caryll_bkblock *bGlyphClassDef = NULL;
	caryll_bkblock *bAttachList = NULL;
	caryll_bkblock *bLigCaretList = NULL;
	caryll_bkblock *bMarkAttachClassDef = NULL;

	if (gdef->glyphClassDef) { bGlyphClassDef = new_bkblock_from_buffer(caryll_write_classdef(gdef->glyphClassDef)); }
	if (gdef->ligCarets && gdef->ligCarets->coverage && gdef->ligCarets->coverage->numGlyphs) {
		bLigCaretList = writeLigCarets(gdef->ligCarets);
	}
	if (gdef->markAttachClassDef) {
		bMarkAttachClassDef = new_bkblock_from_buffer(caryll_write_classdef(gdef->markAttachClassDef));
	}
	caryll_bkblock *root = new_bkblock(b32, 0x10000,             // Version
	                                   p16, bGlyphClassDef,      // GlyphClassDef
	                                   p16, bAttachList,         // AttachList
	                                   p16, bLigCaretList,       // LigCaretList
	                                   p16, bMarkAttachClassDef, // MarkAttachClassDef
	                                   bkover);
	return caryll_write_bk(root);
}
