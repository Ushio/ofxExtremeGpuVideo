//
//  GpuVideoReaderDecompressed.hpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#pragma once

#include "GpuVideoReader.hpp"

#include <memory>
class GpuVideoReaderDecompressed : public IGpuVideoReader {
public:
    GpuVideoReaderDecompressed(std::shared_ptr<IGpuVideoReader> reader);
    
    GpuVideoReaderDecompressed(const GpuVideoReaderDecompressed&) = delete;
    void operator=(const GpuVideoReaderDecompressed&) = delete;
    
    uint32_t getWidth() const { return _width; }
    uint32_t getHeight() const { return _height; }
    uint32_t getFrameCount() const { return _frameCount; }
    float getFramePerSecond() const { return _framePerSecond; }
    GPU_COMPRESS getFormat() const { return _format; }
    uint32_t getFrameBytes() const { return _frameBytes; }
    
    bool isThreadSafe() const { return true; }
    
    void read(uint8_t *dst, int frame) const;
private:
    uint32_t _width = 0;
    uint32_t _height = 0;
    uint32_t _frameCount = 0;
    float _framePerSecond = 0;
    GPU_COMPRESS _format = GPU_COMPRESS_DXT1;
    uint32_t _frameBytes = 0;
    
    std::vector<uint8_t> _decompressed;
};
