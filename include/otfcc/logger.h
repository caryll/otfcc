#ifndef CARYLL_INCLUDE_LOGGER_H
#define CARYLL_INCLUDE_LOGGER_H

#include "dep/sds.h"
#include "caryll/ownership.h"
#include "primitives.h"

typedef struct otfcc_ILoggerTarget {
	void (*dispose)(void *self);             // destructor
	void (*push)(void *self, MOVE sds data); // push data
} otfcc_ILoggerTarget;

typedef enum { log_type_error = 0, log_type_warning = 1, log_type_info = 2, log_type_progress = 3 } otfcc_LoggerType;
enum { log_vl_critical = 0, log_vl_important = 1, log_vl_notice = 2, log_vl_info = 5, log_vl_progress = 10 };

typedef struct otfcc_ILogger {
	void (*dispose)(void *self);                     // destructor
	void (*indent)(void *self, const char *segment); // add a level
	void (*indentSDS)(void *self, MOVE sds segment); // add a level, using SDS
	void (*start)(void *self, const char *segment);  // add a level, output a progress
	void (*startSDS)(void *self, MOVE sds segment);  // add a level, output a progress
	void (*log)(void *self, uint8_t verbosity, otfcc_LoggerType type, const char *data); // log a data
	void (*logSDS)(void *self, uint8_t verbosity, otfcc_LoggerType type, MOVE sds data); // log a data
	void (*dedent)(void *self);                                                          // remove a level
	void (*end)(void *self);                                                             // remove a level
	void (*setVerbosity)(void *self, uint8_t verbosity);                                 // remove a level
	otfcc_ILoggerTarget *(*getTarget)(void *self);                                       // query target
} otfcc_ILogger;

otfcc_ILogger *otfcc_new_Logger(otfcc_ILoggerTarget *target);
otfcc_ILoggerTarget *otfcc_new_StdErrTarget();

#endif
