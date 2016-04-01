//
//  GpuVideoStreamingTexture.cpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#include "GpuVideoStreamingTexture.hpp"

GpuVideoStreamingTexture::GpuVideoStreamingTexture(std::shared_ptr<IGpuVideoReader> reader, GLenum interpolation, GLenum wrap):_reader(reader) {
    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);
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
    
    glGenBuffers(_pbos.size(), _pbos.data());
    for(GLuint pbo : _pbos) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, _reader->getFrameBytes(), 0, GL_STREAM_DRAW);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}
GpuVideoStreamingTexture::~GpuVideoStreamingTexture() {
    glDeleteBuffers(_pbos.size(), _pbos.data());
}
void GpuVideoStreamingTexture::setFrame(int frame) {
    if(_curFrame == frame) {
        return;
    }
    _curFrame = frame;
    
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pbos[_pboIndex]);
    uint8_t *dst = (uint8_t *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    _reader->read(dst, frame);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    
    glBindTexture(GL_TEXTURE_2D, _texture);
    glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0 /* xoffset */, 0 /* yoffset */, _reader->getWidth(), _reader->getHeight(), _glFmt, _reader->getFrameBytes(), 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // インデックスのアップデート
    _pboIndex = (_pboIndex + 1) % _pbos.size();
}