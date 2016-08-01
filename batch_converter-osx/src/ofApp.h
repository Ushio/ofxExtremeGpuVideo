#pragma once

#include "ofMain.h"

#include <memory>
#include <array>

#include <atomic>
#include <future>

#include "ofxImGui.h"

#include "GPUVideo.hpp"
#include "GpuVideoIO.hpp"
#include "GpuVideoReader.hpp"
#include "GpuVideoStreamingTexture.hpp"
#include "GpuVideoOnGpuMemoryTexture.hpp"

class ofApp : public ofBaseApp {
public:
    void setup();
    void exit();
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
    
    bool _isConverting = false;
    
    bool _liteMode = false;
    bool _hasAlpha = false;
    std::vector<std::string> _inputs;
    
    // タスク
    struct ConvTask {
        bool run = false;
        bool done = false;
        
        std::string output_path;
        std::vector<std::string> image_paths;
        std::atomic<int> done_frames;
        std::future<int> work;
    };
    std::vector<std::shared_ptr<ConvTask>> _tasks;
    std::atomic<bool> _abortTask;
    
//    bool _liteMode = false;
//    uint32_t _format = 0;
//    
//    bool _isConverting = false;
//    std::vector<std::string> _imagePaths;
//    std::string _dstPath;
//    uint32_t _width = 0;
//    uint32_t _height = 0;
//    float _fps = 0.0f;
//    uint32_t _bufferSize = 0;
//    uint32_t _squishFlag = 0;
//    
//    std::vector<uint8_t> _gpuCompressBuffer;
//    std::vector<uint8_t> _lz4CompressBuffer;
//    std::vector<Lz4Block> _lz4blocks;
//    
//    int _index = 0;
//    
//	std::unique_ptr<GpuVideoIO> _io;
};
