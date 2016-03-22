#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_maxp(caryll_font *font, caryll_packet packet) {
	FOR_TABLE('maxp', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

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

void caryll_maxp_to_json(caryll_font *font, JSON_Object *root) {
	if (!font->maxp) return;
	json_object_dotset_number(root, "maxp.version", font->maxp->version);
	json_object_dotset_number(root, "maxp.numGlyphs", font->maxp->numGlyphs);
	json_object_dotset_number(root, "maxp.maxPoints", font->maxp->maxPoints);
	json_object_dotset_number(root, "maxp.maxContours", font->maxp->maxContours);
	json_object_dotset_number(root, "maxp.maxCompositePoints", font->maxp->maxCompositePoints);
	json_object_dotset_number(root, "maxp.maxCompositeContours", font->maxp->maxCompositeContours);
	json_object_dotset_number(root, "maxp.maxZones", font->maxp->maxZones);
	json_object_dotset_number(root, "maxp.maxTwilightPoints", font->maxp->maxTwilightPoints);
	json_object_dotset_number(root, "maxp.maxStorage", font->maxp->maxStorage);
	json_object_dotset_number(root, "maxp.maxFunctionDefs", font->maxp->maxFunctionDefs);
	json_object_dotset_number(root, "maxp.maxInstructionDefs", font->maxp->maxInstructionDefs);
	json_object_dotset_number(root, "maxp.maxStackElements", font->maxp->maxStackElements);
	json_object_dotset_number(root, "maxp.maxSizeOfInstructions", font->maxp->maxSizeOfInstructions);
	json_object_dotset_number(root, "maxp.maxComponentElements", font->maxp->maxComponentElements);
	json_object_dotset_number(root, "maxp.maxComponentDepth", font->maxp->maxComponentDepth);
}
