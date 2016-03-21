#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BILLION 1000000000

#ifdef _WIN32
#include <windows.h>
LARGE_INTEGER getFILETIMEoffset() {
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return (t);
}

int clock_gettime(clockid_t X, struct timespec *tv) {
	LARGE_INTEGER t;
	FILETIME f;
	double microseconds;
	static LARGE_INTEGER offset;
	static double frequencyToMicroseconds;
	static int initialized = 0;
	static BOOL usePerformanceCounter = 0;

	if (!initialized) {
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
		usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
		if (usePerformanceCounter) {
			QueryPerformanceCounter(&offset);
			frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.0;
		} else {
			offset = getFILETIMEoffset();
			frequencyToMicroseconds = 10.0;
		}
	}
	if (usePerformanceCounter)
		QueryPerformanceCounter(&t);
	else {
		GetSystemTimeAsFileTime(&f);
		t.QuadPart = f.dwHighDateTime;
		t.QuadPart <<= 32;
		t.QuadPart |= f.dwLowDateTime;
	}

	t.QuadPart -= offset.QuadPart;
	microseconds = (double)t.QuadPart / frequencyToMicroseconds;
	t.QuadPart = microseconds;
	tv->tv_sec = t.QuadPart / 1000000;
	tv->tv_nsec = t.QuadPart % 1000000 * 1000;
	return (0);
}
#endif

#include "caryll-sfnt.h"
#include "caryll-font.h"

#include "extern/parson.h"

void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result) {
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + BILLION;
	} else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
}

void push_stopwatch(const char *reason, struct timespec *sofar) {
	struct timespec ends;
	clock_gettime(CLOCK_REALTIME, &ends);
	struct timespec diff;
	timespec_diff(sofar, &ends, &diff);
	*sofar = ends;
	fprintf(stderr, "%s: %g\n", reason, diff.tv_sec + diff.tv_nsec / (double)BILLION);
}

int main(int argc, char *argv[]) {
	struct timespec begin;
	clock_gettime(CLOCK_REALTIME, &begin);

	caryll_sfnt *sfnt = caryll_sfnt_open(argv[1]);
	caryll_font *font = caryll_font_open(sfnt, 0);

	push_stopwatch("Parse SFNT", &begin);

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);

	caryll_head_to_json(font, root_object);
	caryll_glyf_to_json(font, root_object);

	char *serialized = json_serialize_to_string_pretty(root_value);

	push_stopwatch("Serialize to JSON", &begin);

	puts(serialized);

	push_stopwatch("Write to file", &begin);

	json_free_serialized_string(serialized);
	json_value_free(root_value);

	caryll_font_close(font);
	caryll_sfnt_close(sfnt);
	return 0;
}
