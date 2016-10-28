#ifndef CARYLL_SUPPORT_ALIASES_H
#define CARYLL_SUPPORT_ALIASES_H

#include "otfcc/handle.h"

#define FOR_TABLE(name, table)                                                                                         \
	for (int keep = 1, count = 0, __notfound = 1; __notfound && keep && count < packet.numTables;                      \
	     keep = !keep, count++)                                                                                        \
		for (otfcc_PacketPiece table = (packet.pieces)[count]; keep; keep = !keep)                                    \
			if (table.tag == (name))                                                                                   \
				for (int k2 = 1; k2; k2 = 0, __notfound = 0)

#define foreach_hash(id, range) for (id = (range); id != NULL; id = id->hh.next)

#define loggedStep(...)                                                                                                \
	for (bool ___loggedstep_v =                                                                                        \
	              (options->logger->startSDS(options->logger, sdscatprintf(sdsempty(), __VA_ARGS__)), true);           \
	     ___loggedstep_v; ___loggedstep_v = false, options->logger->dedent(options->logger))
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

typedef uint8_t *font_file_pointer;

#endif
