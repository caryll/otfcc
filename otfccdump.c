#include <stdio.h>
#include <stdlib.h>

#include "caryll-sfnt.h"
#include "caryll-font.h"

#include "extern/parson.h"

#define PATTERN_PROPERTY_NAME "%30s"
#define PATTERN_DEC (PATTERN_PROPERTY_NAME " = %10u\n")
#define PATTERN_SIGNED (PATTERN_PROPERTY_NAME " = %10d\n")
#define PATTERN_HEX (PATTERN_PROPERTY_NAME " = %#010x\n")

#define DUMP_CMAP 0
#define DUMP_GLYF 1
#define DUMP_GLYF_POINTS 0

int main(int argc, char *argv[]) {
	caryll_sfnt *sfnt = caryll_sfnt_open(argv[1]);
	caryll_font *font = caryll_font_open(sfnt, 0);
	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);
	
	caryll_head_to_json(font, root_object);
	char* serialized = json_serialize_to_string_pretty(root_value);
	puts(serialized);
	json_free_serialized_string(serialized);
	json_value_free(root_value);
	
	caryll_font_close(font);
	caryll_sfnt_close(sfnt);
	return 0;
}
