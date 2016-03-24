#include "hmtx.h"

table_hmtx *caryll_read_hmtx(caryll_packet packet, table_hhea *hhea, table_maxp *maxp) {
	if (!hhea || !maxp || hhea->numberOfMetrics == 0) return NULL;
	FOR_TABLE('hmtx', table) {
		font_file_pointer data = table.data;

		table_hmtx *hmtx = (table_hmtx *)malloc(sizeof(table_hmtx) * 1);

		uint32_t count_a = hhea->numberOfMetrics;
		uint32_t count_k = maxp->numGlyphs - hhea->numberOfMetrics;

		hmtx->metrics = (horizontal_metric *)malloc(sizeof(horizontal_metric) * count_a);
		hmtx->leftSideBearing = (int16_t *)malloc(sizeof(int16_t) * count_k);

		for (uint32_t ia = 0; ia < count_a; ia++) {
			hmtx->metrics[ia].advanceWidth = caryll_blt16u(data + ia * 4);
			hmtx->metrics[ia].lsb = caryll_blt16u(data + ia * 4 + 2);
		}

		for (uint32_t ik = 0; ik < count_k; ik++) {
			hmtx->leftSideBearing[ik] = caryll_blt16u(data + count_a * 4 + ik * 2);
		}

		return hmtx;
	}
	return NULL;
}

void caryll_delete_hmtx(table_hmtx *table) {
	if (table->metrics != NULL) free(table->metrics);
	if (table->leftSideBearing != NULL) free(table->leftSideBearing);
	free(table);
}
