#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "extern/json.h"
#include "support/stopwatch.h"

#include "caryll-sfnt.h"
#include "caryll-font.h"

int main(int argc, char *argv[]) {
	char *buffer = 0;
	long length;
	FILE *f = fopen(argv[1], "rb");

	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer) { fread(buffer, 1, length, f); }
		fclose(f);
	}

	if (buffer) {
		json_value *fontdump = json_parse(buffer, length);
		json_value_free(fontdump);
		free(buffer);
	}
	return 0;
}
