#ifndef CARYLL_TESTS_KIT_H
#define CARYLL_TESTS_KIT_H

#define assert_equal(message, x, y)                                                                                    \
	{                                                                                                                  \
		char tmp[16];                                                                                                  \
		sprintf(tmp, "[%d]", ++nChecks);                                                                               \
		printf("%8s Check %s : %s.\n", tmp, message, (x == y ? "Success" : "Fail"));                                   \
	}
#define assert_exists(message, x)                                                                                      \
	{                                                                                                                  \
		char tmp[16];                                                                                                  \
		sprintf(tmp, "[%d]", ++nChecks);                                                                               \
		printf("%8s Check %s : %s.\n", tmp, message, (x != NULL ? "Success" : "Fail"));                                \
	}

#endif
