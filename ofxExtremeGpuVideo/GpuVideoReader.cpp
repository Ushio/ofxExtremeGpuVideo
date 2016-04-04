//
//  GpuVideoReader.cpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/29/16.
//
//

#include "GpuVideoReader.hpp"
#include <cassert>
#include <algorithm>
#include "lz4.h"


GpuVideoReader::GpuVideoReader(const char *path, bool onMemory) {
    _onMemory = onMemory;
    
    _fp = fopen(path, "rb");
    if(_fp == nullptr) {
        throw std::runtime_error("file not found");
    }
    
    fseek(_fp, 0, SEEK_END);
    _rawSize = ftell(_fp);
    fseek(_fp, 0, SEEK_SET);
    
#define R(v) if(fread(&v, 1, sizeof(v), _fp) != sizeof(v)) { assert(0); }
    R(_width);
    R(_height);
    R(_frameCount);
    R(_framePerSecond);
    R(_format);
    R(_frameBytes);
#undef R
    
    // アドレスを読む
    _lz4Blocks.resize(_frameCount);
    size_t s = sizeof(Lz4Block);
    
    fseek(_fp, _rawSize - sizeof(Lz4Block) * _frameCount, SEEK_SET);
    if(fread(_lz4Blocks.data(), 1, sizeof(Lz4Block) * _frameCount, _fp) != sizeof(Lz4Block) * _frameCount) {
        assert(0);
    }
    
    // 必要なら全部読む
    if(_onMemory) {
        _memory.resize(_rawSize);
        fseek(_fp, 0, SEEK_SET);
        if(fread(_memory.data(), 1, _rawSize, _fp) != _rawSize) {
            assert(0);
        }
        fclose(_fp);
        _fp = nullptr;
    } else {
        fseek(_fp, kRawMemoryAt, SEEK_SET);
        
        uint64_t buffer_size = 0;
        for(auto b : _lz4Blocks) {
            buffer_size = std::max(buffer_size, b.size);
        }
        
        _lz4Buffer.resize(buffer_size);
    }
}
GpuVideoReader::~GpuVideoReader() {
    if(_onMemory) {
        
    } else {
        fclose(_fp);
    }
}
void GpuVideoReader::read(uint8_t *dst, int frame) const {
    assert(0 <= frame && frame < _lz4Blocks.size());
    Lz4Block lz4block = _lz4Blocks[frame];
    if(_onMemory) {
        LZ4_decompress_safe((const char *)_memory.data() + lz4block.address, (char *)dst, lz4block.size, _frameBytes);
    } else {
        fseek(_fp, lz4block.address, SEEK_SET);
        if(fread(_lz4Buffer.data(), 1, lz4block.size, _fp) != lz4block.size) {
            assert(0);
        }
        LZ4_decompress_safe((const char *)_lz4Buffer.data(), (char *)dst, lz4block.size, _frameBytes);
    }
}