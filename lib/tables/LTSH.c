#include "support/util.h"
#include "otfcc/table/LTSH.h"

void table_delete_LTSH(table_LTSH *ltsh) {
	if (ltsh) { FREE(ltsh->yPels); }
	FREE(ltsh);
}
table_LTSH *table_read_LTSH(const caryll_Packet packet, const otfcc_Options *options) {
	FOR_TABLE('LTSH', table) {
		font_file_pointer data = table.data;

		table_LTSH *LTSH;
		NEW(LTSH);
		LTSH->version = read_16u(data);
		LTSH->numGlyphs = read_16u(data + 2);
		NEW_N(LTSH->yPels, LTSH->numGlyphs);
		memcpy(LTSH->yPels, data + 4, LTSH->numGlyphs);

		return LTSH;
	}
	return NULL;
}
caryll_Buffer *table_build_LTSH(const table_LTSH *ltsh, const otfcc_Options *options) {
	caryll_Buffer *buf = bufnew();
	if (!ltsh) return buf;
	bufwrite16b(buf, 0);
	bufwrite16b(buf, ltsh->numGlyphs);
	for (uint16_t j = 0; j < ltsh->numGlyphs; j++) {
		bufwrite8(buf, ltsh->yPels[j]);
	}
	return buf;
}
