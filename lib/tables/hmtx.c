#include "support/util.h"
#include "otfcc/table/hmtx.h"

table_hmtx *table_read_hmtx(const caryll_Packet packet, table_hhea *hhea, table_maxp *maxp) {
	if (!hhea || !maxp || !hhea->numberOfMetrics || maxp->numGlyphs < hhea->numberOfMetrics) { return NULL; }
	FOR_TABLE('hmtx', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		table_hmtx *hmtx = NULL;

		glyphid_t count_a = hhea->numberOfMetrics;
		glyphid_t count_k = maxp->numGlyphs - hhea->numberOfMetrics;
		if (length < count_a * 4 + count_k * 2) goto HMTX_CORRUPTED;

		hmtx = malloc(sizeof(table_hmtx) * 1);
		hmtx->metrics = malloc(sizeof(horizontal_metric) * count_a);
		hmtx->leftSideBearing = malloc(sizeof(pos_t) * count_k);

		for (glyphid_t ia = 0; ia < count_a; ia++) {
			hmtx->metrics[ia].advanceWidth = read_16u(data + ia * 4);
			hmtx->metrics[ia].lsb = read_16u(data + ia * 4 + 2);
		}

		for (glyphid_t ik = 0; ik < count_k; ik++) {
			hmtx->leftSideBearing[ik] = read_16u(data + count_a * 4 + ik * 2);
		}

		return hmtx;
	HMTX_CORRUPTED:
		fprintf(stderr, "Table 'hmtx' corrupted.\n");
		if (hmtx) { table_delete_hmtx(hmtx), hmtx = NULL; }
	}
	return NULL;
}

void table_delete_hmtx(table_hmtx *table) {
	if (!table) return;
	if (table->metrics != NULL) free(table->metrics);
	if (table->leftSideBearing != NULL) free(table->leftSideBearing);
	free(table);
}

caryll_Buffer *table_build_hmtx(const table_hmtx *hmtx, glyphid_t count_a, glyphid_t count_k,
                                const otfcc_Options *options) {
	caryll_Buffer *buf = bufnew();
	if (!hmtx) return buf;
	if (hmtx->metrics) {
		for (glyphid_t j = 0; j < count_a; j++) {
			bufwrite16b(buf, hmtx->metrics[j].advanceWidth);
			bufwrite16b(buf, hmtx->metrics[j].lsb);
		}
	}
	if (hmtx->leftSideBearing) {
		for (glyphid_t j = 0; j < count_k; j++) {
			bufwrite16b(buf, hmtx->leftSideBearing[j]);
		}
	}
	return buf;
}
