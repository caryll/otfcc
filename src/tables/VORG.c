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
