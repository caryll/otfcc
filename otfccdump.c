#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "extern/parson.h"
#include "support/stopwatch.h"

#include "caryll-sfnt.h"
#include "caryll-font.h"

int main(int argc, char *argv[]) {
	struct timespec begin;
	time_now(&begin);

	caryll_sfnt *sfnt = caryll_sfnt_open(argv[1]);
	caryll_font *font = caryll_font_open(sfnt, 0);

	push_stopwatch("Parse SFNT", &begin);

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);
	

	caryll_head_to_json(font, root_object);
	caryll_hhea_to_json(font, root_object);
	caryll_maxp_to_json(font, root_object);
	caryll_OS_2_to_json(font, root_object);
	caryll_name_to_json(font, root_object);
	caryll_post_to_json(font, root_object);
	caryll_glyf_to_json(font, root_object);
	caryll_cmap_to_json(font, root_object);
	caryll_glyphorder_to_json(font, root_object);
	
	push_stopwatch("Convert to JSON", &begin);

	char *serialized;
	if (isatty(fileno(stdout))) {
		serialized = json_serialize_to_string_pretty(root_value);
	} else {
		serialized = json_serialize_to_string(root_value);
	}

	push_stopwatch("Serialize to string", &begin);

	fputs(serialized, stdout);

	push_stopwatch("Write to file", &begin);

	json_free_serialized_string(serialized);
	json_value_free(root_value);

	caryll_font_close(font);
	caryll_sfnt_close(sfnt);
	return 0;
}
