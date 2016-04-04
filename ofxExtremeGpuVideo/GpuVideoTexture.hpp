//
//  GpuVideoTexture.hpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#pragma once
#ifdef _MSC_VER
#include <gl/glew.h>
#else
#include <OpenGL/gl.h>
#endif

class IGpuVideoTexture {
public:
    virtual ~IGpuVideoTexture() {}
    
    virtual void setFrame(int frame) = 0;
    virtual GLuint getTexture() const = 0;
};