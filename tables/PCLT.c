#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_PCLT(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'PCLT') {
			uint8_t *data = packet.pieces[i].data;

			table_PCLT *PCLT = (table_PCLT *)malloc(sizeof(table_PCLT) * 1);
			PCLT->version = caryll_blt32u(data);
			PCLT->FontNumber = caryll_blt32u(data + 4);
			PCLT->Pitch = caryll_blt16u(data + 8);
			PCLT->xHeight = caryll_blt16u(data + 10);
			PCLT->Style = caryll_blt16u(data + 12);
			PCLT->TypeFamily = caryll_blt16u(data + 14);
			PCLT->CapHeight = caryll_blt16u(data + 16);
			PCLT->SymbolSet = caryll_blt16u(data + 18);
			memcpy(PCLT->Typeface, data + 20, 16);
			memcpy(PCLT->CharacterComplement, data + 36, 8);
			memcpy(PCLT->FileName, data + 44, 6);
			PCLT->StrokeWeight = *(data + 50);
			PCLT->WidthType = *(data + 51);
			PCLT->SerifStyle = *(data + 52);
			PCLT->pad = 0;

			font->PCLT = PCLT;
		}
	}
}
