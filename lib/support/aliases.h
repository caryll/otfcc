#ifndef CARYLL_SUPPORT_ALIASES_H
#define CARYLL_SUPPORT_ALIASES_H

#include "otfcc/handle.h"

#define send(receiver, method, ...) (receiver)->method(receiver, ##__VA_ARGS__)
#define loggedStep(...)                                                                                                \
	for (bool ___loggedstep_v =                                                                                        \
	              (options->logger->startSDS(options->logger, sdscatprintf(sdsempty(), __VA_ARGS__)), true);           \
	     ___loggedstep_v; ___loggedstep_v = false, send(options->logger, dedent))
#define logError(...)                                                                                                  \
	options->logger->logSDS(options->logger, log_vl_critical, log_type_error, sdscatprintf(sdsempty(), __VA_ARGS__));
#define logWarning(...)                                                                                                \
	options->logger->logSDS(options->logger, log_vl_important, log_type_warning, sdscatprintf(sdsempty(), __VA_ARGS__));
#define logNotice(...)                                                                                                 \
	options->logger->logSDS(options->logger, log_vl_notice, log_type_info, sdscatprintf(sdsempty(), __VA_ARGS__));
#define logProgress(...)                                                                                               \
	options->logger->logSDS(options->logger, log_vl_progress, log_type_progress, sdscatprintf(sdsempty(), __VA_ARGS__));

typedef otfcc_GlyphHandle glyph_handle;
typedef otfcc_FDHandle fd_handle;
typedef otfcc_LookupHandle lookup_handle;

#endif
