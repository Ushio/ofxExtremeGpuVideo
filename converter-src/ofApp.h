#pragma once

#include "ofMain.h"

#include <memory>
#include <array>

#include "ofxImGui.h"

#include "GPUVideo.hpp"
#include "GpuVideoReader.hpp"
#include "GpuVideoStreamingTexture.hpp"
#include "GpuVideoOnGpuMemoryTexture.hpp"

class ofApp : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    void startCompression();
    
    ofxImGui _imgui;
    uint32_t _format = 0;
    
    bool _isConverting = false;
    std::vector<std::string> _imagePaths;
    std::string _dstPath;
    uint32_t _width = 0;
    uint32_t _height = 0;
    float _fps = 0.0f;
    uint32_t _bufferSize = 0;
    uint32_t _squishFlag = 0;
    
    std::vector<uint8_t> _gpuCompressBuffer;
    std::vector<uint8_t> _lz4CompressBuffer;
    std::vector<Lz4Block> _lz4blocks;
    
    int _index = 0;
    
    FILE *_fp = nullptr;
};
