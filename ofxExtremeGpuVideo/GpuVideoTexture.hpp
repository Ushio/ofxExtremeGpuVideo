//
//  GpuVideoTexture.hpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#pragma once
#include <OpenGL/gl.h>

class IGpuVideoTexture {
public:
    virtual ~IGpuVideoTexture() {}
    
    virtual void setFrame(int frame) = 0;
    virtual GLuint getTexture() const = 0;
};