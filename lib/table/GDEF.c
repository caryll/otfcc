#include "GDEF.h"

#include "otl/private.h"

void otfcc_deleteGDEF(table_GDEF *gdef) {
	if (!gdef) return;
	if (gdef->glyphClassDef) ClassDef.dispose(gdef->glyphClassDef);
	if (gdef->markAttachClassDef) ClassDef.dispose(gdef->markAttachClassDef);
	if (gdef->ligCarets) {
		for (glyphid_t j = 0; j < gdef->ligCarets->coverage->numGlyphs; j++) {
			FREE(gdef->ligCarets->carets[j].values);
		}
		FREE(gdef->ligCarets->carets);
		Coverage.dispose(gdef->ligCarets->coverage);
		FREE(gdef->ligCarets);
	}
	FREE(gdef);
}

table_GDEF *otfcc_newGDEF() {
	table_GDEF *gdef;
	NEW(gdef);
	gdef->glyphClassDef = NULL;
	gdef->ligCarets = NULL;
	gdef->markAttachClassDef = NULL;
	return gdef;
}

static otl_CaretValue readCaretValue(const font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_CaretValue v;
	v.format = 0;
	v.coordiante = 0;
	v.pointIndex = 0xFFFF;
	checkLength(offset + 4);

	v.format = read_16u(data + offset);
	if (v.format == 2) { // attach to glyph point
		v.pointIndex = read_16u(data + offset + 2);
	} else {
		v.coordiante = (pos_t)read_16u(data + offset + 2);
	}
FAIL:
	return v;
}
static otl_CaretValueRecord readLigCaretRecord(const font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	checkLength(offset + 2);
	otl_CaretValueRecord g;
	g.values = NULL;
	g.caretCount = read_16u(data + offset);
	checkLength(offset + 2 + g.caretCount * 2);
	NEW(g.values, g.caretCount);

	for (glyphid_t j = 0; j < g.caretCount; j++) {
		g.values[j] = readCaretValue(data, tableLength, offset + read_16u(data + offset + 2 + j * 2));
	}
	return g;
FAIL:
	g.caretCount = 0;
	return g;
}

table_GDEF *otfcc_readGDEF(const otfcc_Packet packet, const otfcc_Options *options) {
	table_GDEF *gdef = NULL;
	FOR_TABLE('GDEF', table) {
		font_file_pointer data = table.data;
		uint32_t tableLength = table.length;
		checkLength(12);
		gdef = otfcc_newGDEF();
		uint16_t classdefOffset = read_16u(data + 4);
		if (classdefOffset) { gdef->glyphClassDef = ClassDef.read(data, tableLength, classdefOffset); }
		uint16_t ligCaretOffset = read_16u(data + 8);
		if (ligCaretOffset) {
			checkLength(ligCaretOffset + 4);
			NEW(gdef->ligCarets);
			gdef->ligCarets->carets = NULL;

			otl_Coverage *cov = Coverage.read(data, tableLength, ligCaretOffset + read_16u(data + ligCaretOffset));
			if (!cov || cov->numGlyphs != read_16u(data + ligCaretOffset + 2)) goto FAIL;
			checkLength(ligCaretOffset + 4 + cov->numGlyphs * 2);
			if (cov->numGlyphs) {
				gdef->ligCarets->coverage = cov;
				NEW(gdef->ligCarets->carets, cov->numGlyphs);
				for (glyphid_t j = 0; j < cov->numGlyphs; j++) {
					gdef->ligCarets->carets[j] = readLigCaretRecord(
					    data, tableLength, ligCaretOffset + read_16u(data + ligCaretOffset + 4 + j * 2));
				}
			} else {
				Coverage.dispose(cov);
				FREE(gdef->ligCarets);
			}
		}
		uint16_t markAttachDefOffset = read_16u(data + 10);
		if (markAttachDefOffset) { gdef->markAttachClassDef = ClassDef.read(data, tableLength, markAttachDefOffset); }
		return gdef;

	FAIL:
		DELETE(otfcc_deleteGDEF, gdef);
	}
	return gdef;
}

static json_value *dumpGDEFLigCarets(const table_GDEF *gdef) {
	json_value *_carets = json_object_new(gdef->ligCarets->coverage->numGlyphs);
	for (glyphid_t j = 0; j < gdef->ligCarets->coverage->numGlyphs; j++) {
		sds name = gdef->ligCarets->coverage->glyphs[j].name;
		json_value *_record = json_array_new(gdef->ligCarets->carets[j].caretCount);

		for (glyphid_t k = 0; k < gdef->ligCarets->carets[j].caretCount; k++) {
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
	return _carets;
}

void otfcc_dumpGDEF(const table_GDEF *gdef, json_value *root, const otfcc_Options *options) {
	if (!gdef) return;
	loggedStep("GDEF") {
		json_value *_gdef = json_object_new(4);
		if (gdef->glyphClassDef) { json_object_push(_gdef, "glyphClassDef", ClassDef.dump(gdef->glyphClassDef)); }
		if (gdef->markAttachClassDef) {
			json_object_push(_gdef, "markAttachClassDef", ClassDef.dump(gdef->markAttachClassDef));
		}
		if (gdef->ligCarets && gdef->ligCarets->coverage && gdef->ligCarets->coverage->numGlyphs) {
			json_object_push(_gdef, "ligCarets", dumpGDEFLigCarets(gdef));
		}
		json_object_push(root, "GDEF", _gdef);
	}
}

static otl_LigCaretTable *ligCaretFromJson(const json_value *_carets) {
	if (!_carets || _carets->type != json_object) return NULL;
	otl_LigCaretTable *lc;
	NEW(lc);
	NEW(lc->coverage);
	lc->coverage->numGlyphs = _carets->u.object.length;
	NEW(lc->coverage->glyphs, lc->coverage->numGlyphs);
	NEW(lc->carets, lc->coverage->numGlyphs);

	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < lc->coverage->numGlyphs; j++) {
		json_value *a = _carets->u.object.values[j].value;
		if (!a || a->type != json_array) continue;
		lc->coverage->glyphs[jj] =
		    Handle.fromName(sdsnewlen(_carets->u.object.values[j].name, _carets->u.object.values[j].name_length));
		lc->carets[jj].caretCount = a->u.array.length;
		NEW(lc->carets[jj].values, lc->carets[jj].caretCount);
		for (glyphid_t k = 0; k < lc->carets[jj].caretCount; k++) {
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

table_GDEF *otfcc_parseGDEF(const json_value *root, const otfcc_Options *options) {
	table_GDEF *gdef = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "GDEF", json_object))) {
		loggedStep("GDEF") {
			gdef = otfcc_newGDEF();
			gdef->glyphClassDef = ClassDef.parse(json_obj_get(table, "glyphClassDef"));
			gdef->markAttachClassDef = ClassDef.parse(json_obj_get(table, "markAttachClassDef"));
			gdef->ligCarets = ligCaretFromJson(json_obj_get(table, "ligCarets"));
		}
	}
	return gdef;
}

static bk_Block *writeLigCaretRec(otl_CaretValueRecord *cr) {
	bk_Block *bcr = bk_new_Block(b16, cr->caretCount, // CaretCount
	                             bkover);
	for (glyphid_t j = 0; j < cr->caretCount; j++) {
		bk_push(bcr, p16,
		        bk_new_Block(b16, cr->values[j].format,                                          // format
		                     b16, cr->values[j].format == 2 ? cr->values[j].pointIndex           // Point index
		                                                    : (int16_t)cr->values[j].coordiante, // X coordinate
		                     bkover),                                                            // CaretValue
		        bkover);
	}
	return bcr;
}

static bk_Block *writeLigCarets(otl_LigCaretTable *lc) {
	bk_Block *lct = bk_new_Block(p16, bk_newBlockFromBuffer(Coverage.build(lc->coverage)), // Coverage
	                             b16, lc->coverage->numGlyphs,                             // LigGlyphCount
	                             bkover);
	for (glyphid_t j = 0; j < lc->coverage->numGlyphs; j++) {
		bk_push(lct, p16, writeLigCaretRec(&(lc->carets[j])), bkover);
	}
	return lct;
}

caryll_Buffer *otfcc_buildGDEF(const table_GDEF *gdef, const otfcc_Options *options) {
	bk_Block *bGlyphClassDef = NULL;
	bk_Block *bAttachList = NULL;
	bk_Block *bLigCaretList = NULL;
	bk_Block *bMarkAttachClassDef = NULL;

	if (gdef->glyphClassDef) { bGlyphClassDef = bk_newBlockFromBuffer(ClassDef.build(gdef->glyphClassDef)); }
	if (gdef->ligCarets && gdef->ligCarets->coverage && gdef->ligCarets->coverage->numGlyphs) {
		bLigCaretList = writeLigCarets(gdef->ligCarets);
	}
	if (gdef->markAttachClassDef) {
		bMarkAttachClassDef = bk_newBlockFromBuffer(ClassDef.build(gdef->markAttachClassDef));
	}
	bk_Block *root = bk_new_Block(b32, 0x10000,             // Version
	                              p16, bGlyphClassDef,      // GlyphClassDef
	                              p16, bAttachList,         // AttachList
	                              p16, bLigCaretList,       // LigCaretList
	                              p16, bMarkAttachClassDef, // MarkAttachClassDef
	                              bkover);
	return bk_build_Block(root);
}
