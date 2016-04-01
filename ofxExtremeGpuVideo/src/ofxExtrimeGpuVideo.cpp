//
//  ofxGpuVideo.cpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#include "ofxExtrimeGpuVideo.hpp"

#include "GpuVideoReader.hpp"
#include "GpuVideoReaderDecompressed.hpp"
#include "GpuVideoStreamingTexture.hpp"
#include "GpuVideoOnGpuMemoryTexture.hpp"

#define GLSL(version, shader)  "#version " #version "\n" #shader

void ofxExtrimeGpuVideo::load(const std::string &name, Mode mode, GLenum interpolation, GLenum wrap) {
    if(_shader.isLoaded() == false) {
        if(ofIsGLProgrammableRenderer()) {
            std::string vs = GLSL(150,
                                  uniform mat4 modelViewProjectionMatrix;
                                  
                                  in vec4 position;
                                  in vec2 texcoord;
                                  
                                  // texture coordinates are sent to fragment shader
                                  out vec2 texCoordVarying;
                                  
                                  void main()
                                  {
                                      texCoordVarying = texcoord;
                                      gl_Position = modelViewProjectionMatrix * position;
                                  }
                                  );
            std::string fs = GLSL(150,
                                  uniform sampler2D u_src;
                                  
                                  // this comes from the vertex shader
                                  in vec2 texCoordVarying;
                                  
                                  // this is the output of the fragment shader
                                  out vec4 outputColor;
                                  
                                  void main()
                                  {
                                      vec4 src = texture(u_src, texCoordVarying);
                                      outputColor = src;
                                  }
                                  );
            _shader.setupShaderFromSource(GL_VERTEX_SHADER, vs);
            _shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fs);
        } else {
            std::string vs = GLSL(120,
                                  varying vec2 v_texcoord;
                                  
                                  void main()
                                  {
                                      v_texcoord = gl_MultiTexCoord0.xy;
                                      gl_Position = ftransform();
                                  }
                                  );
            std::string fs = GLSL(120,
                                  uniform sampler2D u_src;
                                  
                                  varying vec2 v_texcoord;
                                  
                                  void main() {
                                      vec4 src = texture2D(u_src, v_texcoord);
                                      gl_FragColor = src;
                                  }
                                  );
            _shader.setupShaderFromSource(GL_VERTEX_SHADER, vs);
            _shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fs);
        }
        
        if(ofIsGLProgrammableRenderer()){
            _shader.bindDefaults();
        }
        _shader.linkProgram();
    }
    
    static std::shared_ptr<IGpuVideoReader> reader;
    switch(mode) {
        case GPU_VIDEO_STREAMING_FROM_STORAGE: {
            reader = std::make_shared<GpuVideoReader>(ofToDataPath(name).c_str(), false);
            _videoTexture = std::make_shared<GpuVideoStreamingTexture>(reader, interpolation, wrap);
            break;
        }
        case GPU_VIDEO_STREAMING_FROM_CPU_MEMORY: {
            reader = std::make_shared<GpuVideoReader>(ofToDataPath(name).c_str(), true);
            _videoTexture = std::make_shared<GpuVideoStreamingTexture>(reader, interpolation, wrap);
            break;
        }
        case GPU_VIDEO_STREAMING_FROM_CPU_MEMORY_DECOMPRESSED: {
            reader = std::make_shared<GpuVideoReaderDecompressed>(std::make_shared<GpuVideoReader>(ofToDataPath(name).c_str(), false));
            _videoTexture = std::make_shared<GpuVideoStreamingTexture>(reader, interpolation, wrap);
            break;
        }
        case GPU_VIDEO_ON_GPU_MEMORY:
            reader = std::make_shared<GpuVideoReader>(ofToDataPath(name).c_str(), false);
            _videoTexture = std::make_shared<GpuVideoOnGpuMemoryTexture>(reader, interpolation, wrap);
            break;
    }
    
    _width = reader->getWidth();
    _height = reader->getHeight();
    _frameCount = reader->getFrameCount();
    _framePerSecond = reader->getFramePerSecond();
    _placeHolder.allocate(_width, _height, GL_RGB, false);
    
    _isLoaded = true;
}

ofShader ofxExtrimeGpuVideo::_shader;