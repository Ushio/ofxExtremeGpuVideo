//
//  main.c
//  lz4_native_osx
//
//  Created by Nakazi_w0w on 4/1/16.
//  Copyright © 2016 wow. All rights reserved.
//

#include "main.h"

#include "lz4.h"
#include "lz4frame.h"
#include "lz4hc.h"

int lz4_decompress_safe_native(const char* source, char* dest, int compressedSize, int maxDecompressedSize) {
	return LZ4_decompress_safe(source, dest, compressedSize, maxDecompressedSize);
}

int lz4_compress_default_native(const char* source, char* dest, int srcSize, int maxDestSize) {
	return LZ4_compress_default(source, dest, srcSize, maxDestSize);
}

int lz4_compressBound_native(int inputSize) {
	return LZ4_compressBound(inputSize);
}

int lz4_compress_HC_native(const char* source, char* dest, int srcSize, int maxDstSize, int compressionLevel) {
	return LZ4_compress_HC(source, dest, srcSize, maxDstSize, compressionLevel);
}