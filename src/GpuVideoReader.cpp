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
    
	_io = std::make_unique<GpuVideoIO>(path, "rb");
    
	_io->seek(0, SEEK_END);
    _rawSize = _io->tellg();
	_io->seek(0, SEEK_SET);

#define R(v) if(_io->read(&v, sizeof(v)) != sizeof(v)) { assert(0); }
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
    
	_io->seek(_rawSize - sizeof(Lz4Block) * _frameCount, SEEK_SET);
    if(_io->read(_lz4Blocks.data(), sizeof(Lz4Block) * _frameCount) != sizeof(Lz4Block) * _frameCount) {
        assert(0);
    }
    
    // 必要なら全部読む
    if(_onMemory) {
        _memory.resize(_rawSize);
		_io->seek(0, SEEK_SET);
        if(_io->read(_memory.data(), _rawSize) != _rawSize) {
            assert(0);
        }
		_io.reset();
    } else {
		_io->seek(kRawMemoryAt, SEEK_SET);
        
        uint64_t buffer_size = 0;
        for(auto b : _lz4Blocks) {
            buffer_size = std::max(buffer_size, b.size);
        }
        
        _lz4Buffer.resize(buffer_size);
    }
}
GpuVideoReader::~GpuVideoReader() {

}
void GpuVideoReader::read(uint8_t *dst, int frame) const {
    assert(0 <= frame && frame < _lz4Blocks.size());
    Lz4Block lz4block = _lz4Blocks[frame];
    if(_onMemory) {
        LZ4_decompress_safe((const char *)_memory.data() + lz4block.address, (char *)dst, static_cast<int>(lz4block.size), _frameBytes);
    } else {
		_io->seek(lz4block.address, SEEK_SET);
        if(_io->read(_lz4Buffer.data(), lz4block.size) != lz4block.size) {
            assert(0);
        }
        LZ4_decompress_safe((const char *)_lz4Buffer.data(), (char *)dst, static_cast<int>(lz4block.size), _frameBytes);
    }
}