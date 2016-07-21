//
//  GpuVideoStreamingTexture.cpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#include "GpuVideoStreamingTexture.hpp"

GpuVideoStreamingTexture::GpuVideoStreamingTexture(std::shared_ptr<IGpuVideoReader> reader, GLenum interpolation, GLenum wrap):_reader(reader) {

    glGenTextures(2, _textures);
    for(int i = 0 ; i < 2 ; ++i) {
        glBindTexture(GL_TEXTURE_2D, _textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
        switch (_reader->getFormat()) {
            case GPU_COMPRESS_DXT1:
                _glFmt = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                break;
            case GPU_COMPRESS_DXT3:
                _glFmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;
            case GPU_COMPRESS_DXT5:
                _glFmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;
        }
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, _glFmt, _reader->getWidth(), _reader->getHeight(), 0, _reader->getFrameBytes(), nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    _textureMemory.resize(_reader->getFrameBytes());
}
GpuVideoStreamingTexture::~GpuVideoStreamingTexture() {
    glDeleteTextures(2, _textures);
}
void GpuVideoStreamingTexture::updateCPU(int frame) {
    if(_curFrame == frame) {
        return;
    }
    _curFrame = frame;
    
    _reader->read(_textureMemory.data(), frame);
    _textureNeedsUpload = true;
}
void GpuVideoStreamingTexture::uploadGPU() {
    if(_textureNeedsUpload == false) {
        return;
    }
    
    std::swap(_textures[0], _textures[1]);
    glBindTexture(GL_TEXTURE_2D, _textures[0]);
    
    glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0 /* xoffset */, 0 /* yoffset */, _reader->getWidth(), _reader->getHeight(), _glFmt, _reader->getFrameBytes(), _textureMemory.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    
    _textureNeedsUpload = false;
}