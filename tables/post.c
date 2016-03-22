#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

// clang-format off
const char *standardMacNames[258] = {".notdef", ".null", "nonmarkingreturn", "space", "exclam", "quotedbl", "numbersign", "dollar", "percent", "ampersand", "quotesingle", "parenleft", "parenright", "asterisk", "plus", "comma", "hyphen", "period", "slash", "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "colon", "semicolon", "less", "equal", "greater", "question", "at", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "bracketleft", "backslash", "bracketright", "asciicircum", "underscore", "grave", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", "Adieresis", "Aring", "Ccedilla", "Eacute", "Ntilde", "Odieresis", "Udieresis", "aacute", "agrave", "acircumflex", "adieresis", "atilde", "aring", "ccedilla", "eacute", "egrave", "ecircumflex", "edieresis", "iacute", "igrave", "icircumflex", "idieresis", "ntilde", "oacute", "ograve", "ocircumflex", "odieresis", "otilde", "uacute", "ugrave", "ucircumflex", "udieresis", "dagger", "degree", "cent", "sterling", "section", "bullet", "paragraph", "germandbls", "registered", "copyright", "trademark", "acute", "dieresis", "notequal", "AE", "Oslash", "infinity", "plusminus", "lessequal", "greaterequal", "yen", "mu", "partialdiff", "summation", "product", "pi", "integral", "ordfeminine", "ordmasculine", "Omega", "ae", "oslash", "questiondown", "exclamdown", "logicalnot", "radical", "florin", "approxequal", "Delta", "guillemotleft", "guillemotright", "ellipsis", "nonbreakingspace", "Agrave", "Atilde", "Otilde", "OE", "oe", "endash", "emdash", "quotedblleft", "quotedblright", "quoteleft", "quoteright", "divide", "lozenge", "ydieresis", "Ydieresis", "fraction", "currency", "guilsinglleft", "guilsinglright", "fi", "fl", "daggerdbl", "periodcentered", "quotesinglbase", "quotedblbase", "perthousand", "Acircumflex", "Ecircumflex", "Aacute", "Edieresis", "Egrave", "Iacute", "Icircumflex", "Idieresis", "Igrave", "Oacute", "Ocircumflex", "apple", "Ograve", "Uacute", "Ucircumflex", "Ugrave", "dotlessi", "circumflex", "tilde", "macron", "breve", "dotaccent", "ring", "cedilla", "hungarumlaut", "ogonek", "caron", "Lslash", "lslash", "Scaron", "scaron", "Zcaron", "zcaron", "brokenbar", "Eth", "eth", "Yacute", "yacute", "Thorn", "thorn", "minus", "multiply", "onesuperior", "twosuperior", "threesuperior", "onehalf", "onequarter", "threequarters", "franc", "Gbreve", "gbreve", "Idotaccent", "Scedilla", "scedilla", "Cacute", "cacute", "Ccaron", "ccaron", "dcroat"};
// clang-format on

void caryll_read_post(caryll_font *font, caryll_packet packet) {
	FOR_TABLE('post', table) {
		font_file_pointer data = table.data;

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
		post->post_name_map = NULL;
		// Foamt 2 additional glyph names
		if (post->version == 0x20000) {
			glyph_order_hash *map = malloc(sizeof(glyph_order_hash));
			*map = NULL;

			sds pendingNames[0x10000];
			memset(pendingNames, 0, sizeof(pendingNames));
			uint16_t numberGlyphs = caryll_blt16u(data + 32);
			uint32_t offset = 34 + 2 * numberGlyphs;
			uint16_t pendingNameIndex = 0;
			while (pendingNameIndex <= 0xFFFF && offset < table.length) {
				uint8_t len = data[offset];
				sds s;
				if (len > 0) {
					s = sdsnewlen(data + offset + 1, len);
				} else {
					s = sdsempty();
				}
				offset += len + 1;
				pendingNames[pendingNameIndex] = s;
				pendingNameIndex += 1;
			}
			for (uint16_t j = 0; j < numberGlyphs; j++) {
				uint16_t nameMap = caryll_blt16u(data + 34 + 2 * j);
				if (nameMap >= 258) { // Custom glyph name
					try_name_glyph(map, j, sdsdup(pendingNames[nameMap - 258]));
				} else { // Standard Macintosh glyph name
					try_name_glyph(map, j, sdsnew(standardMacNames[nameMap]));
				}
			}
			for (uint32_t j = 0; j < pendingNameIndex; j++) {
				sdsfree(pendingNames[j]);
			}
			post->post_name_map = map;
		}
		font->post = post;
	}
}

void caryll_delete_table_post(caryll_font *font) {
	if (font->post->post_name_map != NULL) { delete_glyph_order_map(font->post->post_name_map); }
	free(font->post);
}


void caryll_post_to_json(caryll_font *font, JSON_Object *root) {
	if(!font->post) return;
	json_object_dotset_number(root, "post.version", font->post->version);
	json_object_dotset_number(root, "post.italicAngle", font->post->italicAngle);
	json_object_dotset_number(root, "post.underlinePosition", font->post->underlinePosition);
	json_object_dotset_number(root, "post.underlineThickness", font->post->underlineThickness);
	json_object_dotset_number(root, "post.isFixedPitch", font->post->isFixedPitch);
	json_object_dotset_number(root, "post.minMemType42", font->post->minMemType42);
	json_object_dotset_number(root, "post.maxMemType42", font->post->maxMemType42);
	json_object_dotset_number(root, "post.minMemType1", font->post->minMemType1);
	json_object_dotset_number(root, "post.maxMemType1", font->post->maxMemType1);
}
