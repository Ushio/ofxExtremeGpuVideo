//
//  GpuVideoOnGpuMemoryTexture.cpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#include "GpuVideoOnGpuMemoryTexture.hpp"

#include <cassert>
GpuVideoOnGpuMemoryTexture::GpuVideoOnGpuMemoryTexture(std::shared_ptr<IGpuVideoReader> reader, GLenum interpolation, GLenum wrap) {
    _textures.resize(reader->getFrameCount());
    glGenTextures(reader->getFrameCount(), _textures.data());
    
    GLuint glFmt = 0;
    switch (reader->getFormat()) {
        case GPU_COMPRESS_DXT1:
            glFmt = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            break;
        case GPU_COMPRESS_DXT3:
            glFmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case GPU_COMPRESS_DXT5:
            glFmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
		case GPU_COMPRESS_BC7:
			glFmt = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
			break;
    }
    std::vector<uint8_t> memory(reader->getFrameBytes());
    for(int i = 0 ; i < _textures.size() ; ++i) {
        GLuint texture = _textures[i];
        
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

        reader->read(memory.data(), i);
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, glFmt, reader->getWidth(), reader->getHeight(), 0, memory.size(), memory.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}
GpuVideoOnGpuMemoryTexture::~GpuVideoOnGpuMemoryTexture() {
    glDeleteBuffers(_textures.size(), _textures.data());
}

void GpuVideoOnGpuMemoryTexture::updateCPU(int frame) {
    assert(0 <= frame && frame < _textures.size());
    _frame = frame;
}
void GpuVideoOnGpuMemoryTexture::uploadGPU() {
    // NOP
}