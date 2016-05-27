//
//  GpuVideoOnGpuMemoryTexture.cpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#ifndef _MSC_VER
#define _FILE_OFFSET_BITS 64
#else
#endif

#include "FileIO.hpp"
#include <stdexcept>

namespace gv {
    FileIO::FileIO(const char *filename, const char *mode) {
        _fp = fopen(filename, mode);
        if (_fp == nullptr) {
            throw std::runtime_error("file not found");
        }
    }
    FileIO::~FileIO() {
        if(_fp) {
            fclose(_fp);
        }
    }
    int FileIO::seek(int64_t offset, int origin) {
#ifdef _MSC_VER
        return _fseeki64(_fp, offset, origin);
#else
        return fseeko(_fp, offset, origin);
#endif
    }
    int64_t FileIO::tellg() {
#ifdef _MSC_VER
        return _ftelli64(_fp);
#else
        return ftello(_fp);
#endif
    }
    int64_t FileIO::read(void *dst, int64_t size) {
        return fread(dst, 1, size, _fp);
    }
    int64_t FileIO::write(const void *src, int64_t size) {
        return fwrite(src, 1, size, _fp);
    }
}
