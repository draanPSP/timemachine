#pragma once

int WriteFile(const char *file, void *buf, int size);
s32 gunzip(u8 *inBuf, u32 inSize, u8 *outBuf, u32 outSize, u32 *realInSize = NULL, bool noHeader = false);
int deflateCompress(void *inbuf, int insize, void *outbuf, int outsize, int level, bool gzip);