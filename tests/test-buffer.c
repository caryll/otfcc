#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../support/buffer.h"

#include "kit.h"

int main(int argc, char *argv[]) {
	printf("Testing Buffer\n");
	int nChecks = 0;
	{ // buffer test
		caryll_buffer *buf = bufnew();
		assert_equal("New buffer's length should be 0", buflen(buf), 0);
		assert_equal("New buffer's length cursor be 0", bufpos(buf), 0);

		bufwrite8(buf, 12);
		assert_equal("After written a byte, buf length should be 1", buflen(buf), 1);
		assert_equal("After written a byte, buf cursor should be 1", bufpos(buf), 1);
		assert_equal("The byte should be written", buf->s[0], 12);

		bufwrite16b(buf, 0x1234);
		bufwrite16l(buf, 0x5678);
		bufwrite32b(buf, 0x01020304);
		bufwrite32l(buf, 0x05060708);

		assert_equal("16BE Byte 2", buf->s[1], 0x12);
		assert_equal("16BE Byte 3", buf->s[2], 0x34);
		assert_equal("16LE Byte 4", buf->s[3], 0x78);
		assert_equal("16LE Byte 5", buf->s[4], 0x56);
		assert_equal("32BE Byte 6", buf->s[5], 0x01);
		assert_equal("32BE Byte 7", buf->s[6], 0x02);
		assert_equal("32BE Byte 8", buf->s[7], 0x03);
		assert_equal("32BE Byte 9", buf->s[8], 0x04);
		assert_equal("32LE Byte 10", buf->s[9], 0x08);
		assert_equal("32LE Byte 11", buf->s[10], 0x07);
		assert_equal("32LE Byte 12", buf->s[11], 0x06);
		assert_equal("32LE Byte 13", buf->s[12], 0x05);
		
		assert_equal("After written them, buf length should be 13", buflen(buf), 13);

		bufseek(buf, 1000);
		sds s = sdsnew("abcd");
		bufwrite_sds(buf, s);
		assert_equal("After written a sds string, buf length should be 1004", buflen(buf), 1004);
		assert_equal("After written a sds string, buf pos should be 1004", bufpos(buf), 1004);
		assert_equal("The string is written", strncmp(buf->s + 1000, "abcd", 4), 0);
		assert_equal("The skipped zone is filled with zero", buf->s[555], 0);
		sdsfree(s);
		
		bufwrite_str(buf, "PBRT");
		assert_equal("After written a string, buf length should be 1004", buflen(buf), 1008);
		assert_equal("After written a string, buf pos should be 1004", bufpos(buf), 1008);
		assert_equal("The string is written", strncmp(buf->s + 1004, "PBRT", 4), 0);
		
		caryll_buffer *that = bufnew();
		sds ts = sdsnew("xyzw");
		bufwrite_sds(that, ts);
		sdsfree(ts);
		bufwrite_buf(buf, that);
		assert_equal("After written a buffer, the buf length should be 1012", buflen(buf), 1012);
		assert_equal("buf written", strncmp(buf->s + 1008, "xyzw", 4), 0);
		buffree(that);

		buffree(buf);
	}
	return 0;
}
