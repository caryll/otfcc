#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_maxp(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'maxp') {
			font_file_pointer data = packet.pieces[i].data;
			uint32_t length = packet.pieces[i].length;

			if (length != 32 && length != 6) {
				printf("table 'maxp' corrupted.\n");
				font->maxp = NULL;
			} else {
				table_maxp *maxp = (table_maxp *)malloc(sizeof(table_maxp) * 1);
				maxp->version = caryll_blt32u(data);
				maxp->numGlyphs = caryll_blt16u(data + 4);
				if (maxp->version == 0x00010000) { // TrueType Format 1
					maxp->maxPoints = caryll_blt16u(data + 6);
					maxp->maxContours = caryll_blt16u(data + 8);
					maxp->maxCompositePoints = caryll_blt16u(data + 10);
					maxp->maxCompositeContours = caryll_blt16u(data + 12);
					maxp->maxZones = caryll_blt16u(data + 14);
					maxp->maxTwilightPoints = caryll_blt16u(data + 16);
					maxp->maxStorage = caryll_blt16u(data + 18);
					maxp->maxFunctionDefs = caryll_blt16u(data + 20);
					maxp->maxInstructionDefs = caryll_blt16u(data + 22);
					maxp->maxStackElements = caryll_blt16u(data + 24);
					maxp->maxSizeOfInstructions = caryll_blt16u(data + 26);
					maxp->maxComponentElements = caryll_blt16u(data + 28);
					maxp->maxComponentDepth = caryll_blt16u(data + 30);
				} else { // CFF OTF Format 0.5
					maxp->maxPoints = 0;
					maxp->maxContours = 0;
					maxp->maxCompositePoints = 0;
					maxp->maxCompositeContours = 0;
					maxp->maxZones = 0;
					maxp->maxTwilightPoints = 0;
					maxp->maxStorage = 0;
					maxp->maxFunctionDefs = 0;
					maxp->maxInstructionDefs = 0;
					maxp->maxStackElements = 0;
					maxp->maxSizeOfInstructions = 0;
					maxp->maxComponentElements = 0;
					maxp->maxComponentDepth = 0;
				}
				font->maxp = maxp;
			}
		}
	}
}
