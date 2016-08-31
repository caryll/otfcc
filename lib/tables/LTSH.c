#include "LTSH.h"

void table_delete_LTSH(table_LTSH *ltsh) {
	if (ltsh) { free(ltsh->yPels); }
	free(ltsh);
}
table_LTSH *table_read_LTSH(caryll_Packet packet) {
	FOR_TABLE('LTSH', table) {
		font_file_pointer data = table.data;

		table_LTSH *LTSH = (table_LTSH *)malloc(sizeof(table_LTSH) * 1);
		LTSH->version = read_16u(data);
		LTSH->numGlyphs = read_16u(data + 2);
		LTSH->yPels = (uint8_t *)malloc(sizeof(uint8_t) * LTSH->numGlyphs);
		memcpy(LTSH->yPels, data + 4, LTSH->numGlyphs);

		return LTSH;
	}
	return NULL;
}
caryll_buffer *table_build_LTSH(table_LTSH *ltsh, const caryll_Options *options) {
	caryll_buffer *buf = bufnew();
	if (!ltsh) return buf;
	bufwrite16b(buf, 0);
	bufwrite16b(buf, ltsh->numGlyphs);
	for (uint16_t j = 0; j < ltsh->numGlyphs; j++) {
		bufwrite8(buf, ltsh->yPels[j]);
	}
	return buf;
}
