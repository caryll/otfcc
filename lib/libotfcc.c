#include "otfcc/sfnt.h"
#include "otfcc/font.h"
#include "otfcc/sfnt-builder.h"

int otfccbuild_json_otf(uint32_t inlen, const char *injson, size_t *outlen, uint8_t **outotf) {
	otfcc_Options *options = otfcc_newOptions();
	options->logger = otfcc_newLogger(otfcc_newEmptyTarget());
	options->logger->indent(options->logger, "otfccbuild");
	otfcc_Options_optimizeTo(options, 1);

	// json parsing
	json_value *jsonRoot = json_parse(injson, inlen);
	if (!jsonRoot) { return 1; }
	// font parsing
	otfcc_IFontBuilder *parser = otfcc_newJsonReader();
	otfcc_Font *font = parser->read(jsonRoot, 0, options);
	parser->free(parser);
	json_value_free(jsonRoot);
	if (!font) { return 1; }

	// consolidation and build
	otfcc_iFont.consolidate(font, options);
	otfcc_IFontSerializer *writer = otfcc_newOTFWriter();
	caryll_Buffer *otf = (caryll_Buffer *)writer->serialize(font, options);
	*outlen = buflen(otf);
	*outotf = malloc(*outlen);
	memcpy(*outotf, otf->data, *outlen);
	buffree(otf), writer->free(writer), otfcc_iFont.free(font);
	return 0;
}

void otfccbuild_free_otfbuf(uint8_t *buf) {
	free(buf);
}
