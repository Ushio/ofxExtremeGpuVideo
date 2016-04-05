//
//  GpuVideoStreamingTexture.hpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#include <memory>
#include <array>
#ifdef _MSC_VER
#include <gl/glew.h>
#else
#include <OpenGL/gl.h>
#endif

#include "GpuVideoTexture.hpp"
#include "GpuVideoReader.hpp"

/**
 * CPUメモリ、またはストレージからのストリーミングを行う
 */
class GpuVideoStreamingTexture : public IGpuVideoTexture {
public:
    GpuVideoStreamingTexture(std::shared_ptr<IGpuVideoReader> reader, GLenum interpolation = GL_LINEAR, GLenum wrap = GL_CLAMP_TO_EDGE, bool usePBO = false);
    ~GpuVideoStreamingTexture();
    
    GpuVideoStreamingTexture(const GpuVideoStreamingTexture&) = delete;
    void operator=(const GpuVideoStreamingTexture&) = delete;
    
    void setFrame(int frame);
    GLuint getTexture() const { return _texture; }
private:
    std::shared_ptr<IGpuVideoReader> _reader;
    GLuint _texture = 0;
    std::array<GLuint, 3> _pbos;
    GLuint _glFmt = 0;
    int _pboIndex = 0;
    int _curFrame = -1;

	bool _usePBO = false;
	std::vector<uint8_t> _buffer;
};