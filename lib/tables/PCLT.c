#include "PCLT.h"

table_PCLT *caryll_read_PCLT(caryll_Packet packet) {
	FOR_TABLE('PCLT', table) {
		font_file_pointer data = table.data;

		table_PCLT *PCLT = (table_PCLT *)malloc(sizeof(table_PCLT) * 1);
		PCLT->version = read_32u(data);
		PCLT->FontNumber = read_32u(data + 4);
		PCLT->Pitch = read_16u(data + 8);
		PCLT->xHeight = read_16u(data + 10);
		PCLT->Style = read_16u(data + 12);
		PCLT->TypeFamily = read_16u(data + 14);
		PCLT->CapHeight = read_16u(data + 16);
		PCLT->SymbolSet = read_16u(data + 18);
		memcpy(PCLT->Typeface, data + 20, 16);
		memcpy(PCLT->CharacterComplement, data + 36, 8);
		memcpy(PCLT->FileName, data + 44, 6);
		PCLT->StrokeWeight = *(data + 50);
		PCLT->WidthType = *(data + 51);
		PCLT->SerifStyle = *(data + 52);
		PCLT->pad = 0;

		return PCLT;
	}
	return NULL;
}
