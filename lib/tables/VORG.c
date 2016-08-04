#include "VORG.h"
table_VORG *caryll_read_VORG(caryll_packet packet) {
	FOR_TABLE('VORG', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 8) return NULL;
		uint16_t numVertOriginYMetrics = read_16u(data + 6);
		if (length < 8 + 4 * numVertOriginYMetrics) return NULL;

		table_VORG *vorg;
		NEW(vorg);
		vorg->defaultVerticalOrigin = read_16s(data + 4);
		vorg->numVertOriginYMetrics = numVertOriginYMetrics;
		NEW_N(vorg->entries, numVertOriginYMetrics);
		for (uint16_t j = 0; j < numVertOriginYMetrics; j++) {
			vorg->entries[j].gid = read_16u(data + 8 + 4 * j);
			vorg->entries[j].verticalOrigin = read_16s(data + 8 + 4 * j + 2);
		}
		return vorg;
	}
	return NULL;
}

void caryll_delete_VORG(table_VORG *vorg) {
	if (vorg) free(vorg->entries);
	free(vorg);
}

caryll_buffer *caryll_write_VORG(table_VORG *table, const caryll_options *options) {
	caryll_buffer *buf = bufnew();
	if (!table) return buf;
	bufwrite16b(buf, 1);
	bufwrite16b(buf, 0);
	bufwrite16b(buf, table->defaultVerticalOrigin);
	bufwrite16b(buf, table->numVertOriginYMetrics);
	for (uint16_t j = 0; j < table->numVertOriginYMetrics; j++) {
		bufwrite16b(buf, table->entries[j].gid);
		bufwrite16b(buf, table->entries[j].verticalOrigin);
	}
	return buf;
}
