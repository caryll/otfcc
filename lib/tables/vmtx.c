#include "support/util.h"
#include "otfcc/table/vmtx.h"

void table_delete_vmtx(MOVE table_vmtx *table) {
	if (!table) return;
	if (table->metrics != NULL) FREE(table->metrics);
	if (table->topSideBearing != NULL) FREE(table->topSideBearing);
	FREE(table);
}

table_vmtx *table_read_vmtx(const caryll_Packet packet, const otfcc_Options *options, table_vhea *vhea,
                            table_maxp *maxp) {
	if (!vhea || !maxp || vhea->numOfLongVerMetrics == 0 || maxp->numGlyphs < vhea->numOfLongVerMetrics) return NULL;
	FOR_TABLE('vmtx', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		table_vmtx *vmtx = NULL;

		glyphid_t count_a = vhea->numOfLongVerMetrics;
		glyphid_t count_k = maxp->numGlyphs - vhea->numOfLongVerMetrics;
		if (length < count_a * 4 + count_k * 2) goto vmtx_CORRUPTED;

		NEW(vmtx);
		NEW(vmtx->metrics, count_a);
		NEW(vmtx->topSideBearing, count_k);

		for (glyphid_t ia = 0; ia < count_a; ia++) {
			vmtx->metrics[ia].advanceHeight = read_16u(data + ia * 4);
			vmtx->metrics[ia].tsb = read_16u(data + ia * 4 + 2);
		}

		for (glyphid_t ik = 0; ik < count_k; ik++) {
			vmtx->topSideBearing[ik] = read_16u(data + count_a * 4 + ik * 2);
		}

		return vmtx;
	vmtx_CORRUPTED:
		logWarning("Table 'vmtx' corrupted.\n");
		if (vmtx) { table_delete_vmtx(vmtx), vmtx = NULL; }
	}
	return NULL;
}

caryll_Buffer *table_build_vmtx(const table_vmtx *vmtx, glyphid_t count_a, glyphid_t count_k,
                                const otfcc_Options *options) {
	caryll_Buffer *buf = bufnew();
	if (!vmtx) return buf;
	if (vmtx->metrics) {
		for (glyphid_t j = 0; j < count_a; j++) {
			bufwrite16b(buf, vmtx->metrics[j].advanceHeight);
			bufwrite16b(buf, vmtx->metrics[j].tsb);
		}
	}
	if (vmtx->topSideBearing) {
		for (glyphid_t j = 0; j < count_k; j++) {
			bufwrite16b(buf, vmtx->topSideBearing[j]);
		}
	}
	return buf;
}
