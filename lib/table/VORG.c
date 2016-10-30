#include "VORG.h"

#include "support/util.h"

void otfcc_deleteVORG(table_VORG *vorg) {
	if (vorg) FREE(vorg->entries);
	FREE(vorg);
}

table_VORG *otfcc_readVORG(const otfcc_Packet packet, const otfcc_Options *options) {
	FOR_TABLE('VORG', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 8) goto VORG_CORRUPTED;
		uint16_t numVertOriginYMetrics = read_16u(data + 6);
		if (length < 8 + 4 * numVertOriginYMetrics) goto VORG_CORRUPTED;

		table_VORG *vorg;
		NEW(vorg);
		vorg->defaultVerticalOrigin = read_16s(data + 4);
		vorg->numVertOriginYMetrics = numVertOriginYMetrics;
		NEW(vorg->entries, numVertOriginYMetrics);
		for (uint16_t j = 0; j < numVertOriginYMetrics; j++) {
			vorg->entries[j].gid = read_16u(data + 8 + 4 * j);
			vorg->entries[j].verticalOrigin = read_16s(data + 8 + 4 * j + 2);
		}
		return vorg;
	VORG_CORRUPTED:
		logWarning("Table 'VORG' corrupted.");
	}
	return NULL;
}

caryll_Buffer *otfcc_buildVORG(const table_VORG *table, const otfcc_Options *options) {
	caryll_Buffer *buf = bufnew();
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
