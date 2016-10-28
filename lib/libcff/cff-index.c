#include "cff-index.h"
// INDEX util functions
cff_Index *cff_new_Index(void) {
	cff_Index *out;
	NEW(out);
	return out;
}

void cff_close_Index(cff_Index in) {
	if (in.offset != NULL) FREE(in.offset);
	if (in.data != NULL) FREE(in.data);
}

void cff_empty_Index(cff_Index *in) {
	in->count = 0;
	in->offSize = 0;
	in->offset = NULL;
	in->data = NULL;
}

void cff_delete_Index(cff_Index *out) {
	if (out != NULL) {
		if (out->offset != NULL) FREE(out->offset);
		if (out->data != NULL) FREE(out->data);
		FREE(out);
	}
}

uint32_t cff_lengthOfIndex(cff_Index i) {
	if (i.count != 0)
		return 3 + (i.offset[i.count] - 1) + ((i.count + 1) * i.offSize);
	else
		return 3;
}

void cff_extract_Index(uint8_t *data, uint32_t pos, cff_Index *in) {
	in->count = gu2(data, pos);
	in->offSize = gu1(data, pos + 2);

	if (in->count > 0) {
		NEW(in->offset, in->count + 1);

		for (int i = 0; i <= in->count; i++) {
			switch (in->offSize) {
				case 1:
					in->offset[i] = gu1(data, pos + 3 + (i * in->offSize));
					break;
				case 2:
					in->offset[i] = gu2(data, pos + 3 + (i * in->offSize));
					break;
				case 3:
					in->offset[i] = gu3(data, pos + 3 + (i * in->offSize));
					break;
				case 4:
					in->offset[i] = gu4(data, pos + 3 + (i * in->offSize));
					break;
			}
		}

		NEW(in->data, in->offset[in->count] - 1);
		memcpy(in->data, data + pos + 3 + (in->count + 1) * in->offSize, in->offset[in->count] - 1);
	} else {
		in->offset = NULL;
		in->data = NULL;
	}
}

cff_Index *cff_newIndexByCallback(void *context, uint32_t length, caryll_Buffer *(*fn)(void *, uint32_t)) {
	cff_Index *idx = cff_new_Index();
	idx->count = length;
	NEW(idx->offset, idx->count + 1);
	idx->offset[0] = 1;
	idx->data = NULL;

	size_t used = 0;
	size_t blank = 0;
	for (uint32_t i = 0; i < length; i++) {
		caryll_Buffer *blob = fn(context, i);
		if (blank < blob->size) {
			used += blob->size;
			blank = (used >> 1) & 0xFFFFFF;
			RESIZE(idx->data, used + blank);
		} else {
			used += blob->size;
			blank -= blob->size;
		}
		idx->offset[i + 1] = (uint32_t)(blob->size + idx->offset[i]);
		memcpy(idx->data + idx->offset[i] - 1, blob->data, blob->size);
		buffree(blob);
	}
	idx->offSize = 4;
	return idx;
}

caryll_Buffer *cff_build_Index(cff_Index index) {
	caryll_Buffer *blob = bufnew();
	if (!index.count) {
		bufwrite8(blob, 0);
		bufwrite8(blob, 0);
		bufwrite8(blob, 0);
		return blob;
	}
	uint32_t i;

	uint32_t lastOffset = index.offset[index.count];
	if (lastOffset < 0x100) {
		index.offSize = 1;
	} else if (lastOffset < 0x10000) {
		index.offSize = 2;
	} else if (lastOffset < 0x1000000) {
		index.offSize = 3;
	} else {
		index.offSize = 4;
	}

	if (index.count != 0)
		blob->size = 3 + (index.offset[index.count] - 1) + ((index.count + 1) * index.offSize);
	else
		blob->size = 3;

	NEW(blob->data, blob->size);
	blob->data[0] = index.count / 256;
	blob->data[1] = index.count % 256;
	blob->data[2] = index.offSize;

	if (index.count > 0) {
		for (i = 0; i <= index.count; i++) {
			switch (index.offSize) {
				case 1:
					blob->data[3 + i] = index.offset[i];
					break;
				case 2:
					blob->data[3 + i * 2] = index.offset[i] / 256;
					blob->data[4 + i * 2] = index.offset[i] % 256;
					break;
				case 3:
					blob->data[3 + i * 3] = index.offset[i] / 65536;
					blob->data[4 + i * 3] = (index.offset[i] % 65536) / 256;
					blob->data[5 + i * 3] = (index.offset[i] % 65536) % 256;
					break;
				case 4:
					blob->data[3 + i * 4] = (index.offset[i] / 65536) / 256;
					blob->data[4 + i * 4] = (index.offset[i] / 65536) % 256;
					blob->data[5 + i * 4] = (index.offset[i] % 65536) / 256;
					blob->data[6 + i * 4] = (index.offset[i] % 65536) % 256;
					break;
			}
		}

		if (index.data != NULL)
			memcpy(blob->data + 3 + ((index.count + 1) * index.offSize), index.data, index.offset[index.count] - 1);
	}
	blob->cursor = blob->size;
	return blob;
}
