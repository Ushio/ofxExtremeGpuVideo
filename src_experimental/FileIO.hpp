//
//  GpuVideoOnGpuMemoryTexture.hpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#pragma once

#include <cstdio>
#include <cstdint>

namespace gv {
    class FileIO {
    public:
        FileIO(const char *filename, const char *mode);
        ~FileIO();
        
        FileIO(const FileIO &) = delete;
        void operator=(const FileIO &) = delete;
        
        int seek(int64_t offset, int origin);
        int64_t tellg();
        int64_t read(void *dst, int64_t size);
        int64_t write(const void *src, int64_t size);
    private:
        FILE *_fp = nullptr;
    };
}