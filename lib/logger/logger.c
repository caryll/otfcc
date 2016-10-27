#include "support/util.h"
#include "otfcc/logger.h"

typedef struct Logger {
	otfcc_ILogger vtable;
	void *target;
	uint16_t level;
	uint16_t lastLoggedLevel;
	uint16_t levelCap;
	sds *indents;
	uint8_t verbosityLimit;
} Logger;

const char *otfcc_LoggerTypeNames[3] = {"[ERROR]", "[WARNING]", "[NOTE]"};

static void loggerIndent(void *_self, const char *segment) {
	((otfcc_ILogger *)_self)->indentSDS(_self, sdsnew(segment));
}
static void loggerIndentSDS(void *_self, MOVE sds segment) {
	Logger *self = (Logger *)_self;
	uint8_t newLevel = self->level + 1;
	if (newLevel > self->levelCap) {
		self->levelCap += self->levelCap / 2 + 1;
		if (self->indents) {
			self->indents = realloc(self->indents, sizeof(sds) * self->levelCap);
		} else {
			self->indents = calloc(sizeof(sds), self->levelCap);
		}
	}
	self->level++;
	self->indents[self->level - 1] = segment;
}

static void loggerDedent(void *_self) {
	Logger *self = (Logger *)_self;
	if (!self->level) return;
	sdsfree(self->indents[self->level - 1]);
	self->level -= 1;
	if (self->level < self->lastLoggedLevel) { self->lastLoggedLevel = self->level; }
}
static void loggerStart(void *_self, const char *segment) {
	((otfcc_ILogger *)_self)
	    ->logSDS(_self, log_vl_progress + ((Logger *)_self)->level, log_type_progress, sdsnew(segment));
	((otfcc_ILogger *)_self)->indentSDS(_self, sdsnew(segment));
}
static void loggerStartSDS(void *_self, MOVE sds segment) {
	((otfcc_ILogger *)_self)
	    ->logSDS(_self, log_vl_progress + ((Logger *)_self)->level, log_type_progress, sdsdup(segment));
	((otfcc_ILogger *)_self)->indentSDS(_self, segment);
}
static void loggerLog(void *_self, uint8_t verbosity, otfcc_LoggerType type, const char *data) {
	((otfcc_ILogger *)_self)->logSDS(_self, verbosity, type, sdsnew(data));
}

static void loggerLogSDS(void *_self, uint8_t verbosity, otfcc_LoggerType type, MOVE sds data) {
	Logger *self = (Logger *)_self;
	sds demand = sdsempty();
	for (uint16_t level = 0; level < self->level; level++) {
		if (level < self->lastLoggedLevel - 1) {
			demand = sdscat(demand, " .  ");
		} else {
			demand = sdscatfmt(demand, "%S : ", self->indents[level]);
		}
	}
	if (type < 3) {
		demand = sdscatfmt(demand, "%s %S", otfcc_LoggerTypeNames[type], data);
	} else {
		demand = sdscatfmt(demand, "%S", data);
	}
	sdsfree(data);
	if (verbosity <= self->verbosityLimit) {
		((otfcc_ILogger *)self)->getTarget(self)->push(self->target, demand);
		self->lastLoggedLevel = self->level;
	} else {
		sdsfree(demand);
	}
}

static otfcc_ILoggerTarget *loggerGetTarget(void *_self) {
	Logger *self = (Logger *)_self;
	return (otfcc_ILoggerTarget *)self->target;
}

static void loggerSetVerbosity(void *_self, uint8_t verbosity) {
	Logger *self = (Logger *)_self;
	self->verbosityLimit = verbosity;
}

static void loggerDispose(void *_self) {
	Logger *self = (Logger *)_self;
	if (!self) return;
	otfcc_ILoggerTarget *target = ((otfcc_ILogger *)self)->getTarget(self);
	target->dispose(target);
	for (uint16_t level = 0; level < self->level; level++) {
		sdsfree(self->indents[level]);
	}
	free(self->indents);
	free(self);
}

const otfcc_ILogger VTABLE_LOGGER = {.dispose = loggerDispose,
                                     .indent = loggerIndent,
                                     .indentSDS = loggerIndentSDS,
                                     .dedent = loggerDedent,
                                     .start = loggerStart,
                                     .startSDS = loggerStartSDS,
                                     .log = loggerLog,
                                     .logSDS = loggerLogSDS,
                                     .getTarget = loggerGetTarget,
                                     .setVerbosity = loggerSetVerbosity};

otfcc_ILogger *otfcc_new_Logger(otfcc_ILoggerTarget *target) {
	Logger *logger;
	NEW(logger);
	logger->target = target;
	logger->vtable = VTABLE_LOGGER;
	return (otfcc_ILogger *)logger;
}

// STDERR logger target

typedef struct StderrTarget { otfcc_ILoggerTarget vtable; } StderrTarget;

void stderrTargetDispose(void *_self) {
	StderrTarget *self = (StderrTarget *)_self;
	if (!self) return;
	free(self);
}

void stderrTargetPush(void *_self, MOVE sds data) {
	fprintf(stderr, "%s", data);
	if (data[sdslen(data) - 1] != '\n') fprintf(stderr, "\n");
	sdsfree(data);
}

const otfcc_ILoggerTarget VTABLE_STDERR_TARGET = {.dispose = stderrTargetDispose, .push = stderrTargetPush};

otfcc_ILoggerTarget *otfcc_new_StdErrTarget() {
	StderrTarget *target;
	NEW(target);
	target->vtable = VTABLE_STDERR_TARGET;
	return (otfcc_ILoggerTarget *)target;
}
