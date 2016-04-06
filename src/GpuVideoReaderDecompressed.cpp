//
//  GpuVideoReaderDecompressed.cpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#include "GpuVideoReaderDecompressed.hpp"

GpuVideoReaderDecompressed::GpuVideoReaderDecompressed(std::shared_ptr<IGpuVideoReader> reader) {
    _width = reader->getWidth();
    _height = reader->getHeight();
    _frameCount = reader->getFrameCount();
    _framePerSecond = reader->getFramePerSecond();
    _format = reader->getFormat();
    _frameBytes = reader->getFrameBytes();
    
    _decompressed.resize(_frameCount * _frameBytes);
    for(uint32_t i = 0 ; i < _frameCount ; ++i) {
        reader->read(_decompressed.data() + i * _frameBytes, i);
    }
}

void GpuVideoReaderDecompressed::read(uint8_t *dst, int frame) const {
    memcpy(dst, _decompressed.data() + frame * _frameBytes, _frameBytes);
}