#include "vmtx.h"

table_vmtx *caryll_read_vmtx(caryll_packet packet, table_vhea *vhea, table_maxp *maxp) {
	if (!vhea || !maxp || vhea->numOfLongVerMetrics == 0 || maxp->numGlyphs < vhea->numOfLongVerMetrics) return NULL;
	FOR_TABLE('vmtx', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		table_vmtx *vmtx = NULL;

		uint32_t count_a = vhea->numOfLongVerMetrics;
		uint32_t count_k = maxp->numGlyphs - vhea->numOfLongVerMetrics;
		if (length < count_a * 4 + count_k * 2) goto vmtx_CORRUPTED;

		vmtx = malloc(sizeof(table_vmtx) * 1);
		vmtx->metrics = malloc(sizeof(vertical_metric) * count_a);
		vmtx->topSideBearing = malloc(sizeof(int16_t) * count_k);

		for (uint32_t ia = 0; ia < count_a; ia++) {
			vmtx->metrics[ia].advanceHeight = read_16u(data + ia * 4);
			vmtx->metrics[ia].tsb = read_16u(data + ia * 4 + 2);
		}

		for (uint32_t ik = 0; ik < count_k; ik++) { vmtx->topSideBearing[ik] = read_16u(data + count_a * 4 + ik * 2); }

		return vmtx;
	vmtx_CORRUPTED:
		fprintf(stderr, "Table 'vmtx' corrupted.\n");
		if (vmtx) { caryll_delete_vmtx(vmtx), vmtx = NULL; }
	}
	return NULL;
}

void caryll_delete_vmtx(table_vmtx *table) {
	if (!table) return;
	if (table->metrics != NULL) free(table->metrics);
	if (table->topSideBearing != NULL) free(table->topSideBearing);
	free(table);
}

caryll_buffer *caryll_write_vmtx(table_vmtx *vmtx, uint16_t count_a, uint16_t count_k, caryll_options *options) {
	caryll_buffer *buf = bufnew();
	if (!vmtx) return buf;
	if (vmtx->metrics) {
		for (uint16_t j = 0; j < count_a; j++) {
			bufwrite16b(buf, vmtx->metrics[j].advanceHeight);
			bufwrite16b(buf, vmtx->metrics[j].tsb);
		}
	}
	if (vmtx->topSideBearing) {
		for (uint16_t j = 0; j < count_k; j++) { bufwrite16b(buf, vmtx->topSideBearing[j]); }
	}
	return buf;
}
