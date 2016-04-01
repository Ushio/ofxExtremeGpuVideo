//
//  main.c
//  lz4_native_osx
//
//  Created by Nakazi_w0w on 4/1/16.
//  Copyright © 2016 wow. All rights reserved.
//

#include "main.h"

#include "lz4.h"

int lz4_decompress_safe_native(const char* source, char* dest, int compressedSize, int maxDecompressedSize) {
    return LZ4_decompress_safe(source, dest, compressedSize, maxDecompressedSize);
}