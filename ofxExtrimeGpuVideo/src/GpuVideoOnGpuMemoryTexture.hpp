//
//  GpuVideoOnGpuMemoryTexture.hpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#pragma once

#include <memory>
#include <array>
#include <vector>
#include <OpenGL/gl.h>

#include "GpuVideoTexture.hpp"
#include "GpuVideoReader.hpp"

/**
 * GPUメモリにすべてロードする
 */
class GpuVideoOnGpuMemoryTexture : public IGpuVideoTexture {
public:
    GpuVideoOnGpuMemoryTexture(std::shared_ptr<IGpuVideoReader> reader, GLenum interpolation = GL_LINEAR, GLenum wrap = GL_CLAMP_TO_EDGE);
    ~GpuVideoOnGpuMemoryTexture();
    
    GpuVideoOnGpuMemoryTexture(const GpuVideoOnGpuMemoryTexture&) = delete;
    void operator=(const GpuVideoOnGpuMemoryTexture&) = delete;
    
    void setFrame(int frame);
    GLuint getTexture() const { return _textures[_frame]; }
private:
    int _frame = 0;
    std::vector<GLuint> _textures;
};