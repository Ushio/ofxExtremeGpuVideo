//
//  ofxGpuVideo.hpp
//  emptyExample
//
//  Created by Nakazi_w0w on 3/30/16.
//
//

#pragma once

#include <memory>

#include "ofMain.h"
#include "GpuVideoTexture.hpp"

class ofxExtremeGpuVideo {
public:
    enum Mode {
        /* streaming from storage */
        GPU_VIDEO_STREAMING_FROM_STORAGE,
        
        /* on memory streaming */
        GPU_VIDEO_STREAMING_FROM_CPU_MEMORY,
        
        /* on memory streaming with pre decompressed */
        GPU_VIDEO_STREAMING_FROM_CPU_MEMORY_DECOMPRESSED,
        
        /* all gpu texture */
        GPU_VIDEO_ON_GPU_MEMORY
    };
    void load(const std::string &name, Mode mode, GLenum interpolation = GL_LINEAR, GLenum wrap = GL_CLAMP_TO_EDGE);
    bool isLoaded() const {
        return _isLoaded;
    }
    uint32_t getWidth() const { return _width; }
    uint32_t getHeight() const { return _height; }
    uint32_t getFrameCount() const { return _frameCount; }
    float getFramePerSecond() const { return _framePerSecond; }
    
    float getDuration() const {
        return _frameCount * (1.0f / _framePerSecond);
    }
    
    void setTime(float atTime) {
        float framef = atTime * _framePerSecond;
        this->setFrame(static_cast<int>(framef));
    }
    void setFrame(int frameAt) {
        frameAt = std::max(frameAt, 0);
        frameAt = std::min(frameAt, (int)_frameCount - 1);
        _frameAt = frameAt;
    }
    
    // update (simple use)
    void update() {
        if(_isLoaded == false) {
            return;
        }
        _videoTexture->updateCPU(_frameAt);
        _videoTexture->uploadGPU();
    }
    
    // separate update (advanced use)
    void updateCPU() {
        _videoTexture->updateCPU(_frameAt);
    }
    void uploadGPU() {
        _videoTexture->uploadGPU();
    }
    
    // warning! this texture is Placeholder. It's fake texture
    void begin() {
        if(_isLoaded == false) {
            return;
        }
        _shader.begin();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _videoTexture->getTexture());
        _shader.setUniform1i("u_src", 1);
    }
    GLuint getRawTexture() const {
        return _isLoaded ? _videoTexture->getTexture() : 0;
    }
    ofTexture &getPlaceHolderTexture() {
        return _placeHolder;
    }
    void end() {
        if(_isLoaded == false) {
            return;
        }
        _shader.end();
    }
private:
    bool _isLoaded = false;
    int _frameAt = 0;
    
    uint32_t _width = 0;
    uint32_t _height = 0;
    uint32_t _frameCount = 0;
    float _framePerSecond = 0;
    
    std::shared_ptr<IGpuVideoTexture> _videoTexture;
    
    ofTexture _placeHolder;
    static ofShader _shader;
};