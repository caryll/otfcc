#ifndef CARYLL_TESTS_KIT_H
#define CARYLL_TESTS_KIT_H

#define assert_equal(message, x, y)                                                                                    \
	{                                                                                                                  \
		char tmp[16];                                                                                                  \
		sprintf(tmp, "[%d]", ++nChecks);                                                                               \
		printf("%8s [%s] Check %s : %s.\n", tmp, ((x) == (y) ? "OK" : "FAIL"), message,                                \
		       ((x) == (y) ? "Success" : "Fail"));                                                                     \
	}
#define assert_not_equal(message, x, y)                                                                                \
	{                                                                                                                  \
		char tmp[16];                                                                                                  \
		sprintf(tmp, "[%d]", ++nChecks);                                                                               \
		printf("%8s [%s] Check %s : %s.\n", tmp, ((x) != (y) ? "OK" : "FAIL"), message,                                \
		       ((x) != (y) ? "Success" : "Fail"));                                                                     \
	}
#define assert_exists(message, x) assert_not_equal(message, x, NULL)

#endif
