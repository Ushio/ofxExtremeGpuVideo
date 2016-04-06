#include "ofApp.h"

#ifdef _MSC_VER
#ifdef _DEBUG
#define TBB_LIB_EXT "_debug.lib"
#else
#define TBB_LIB_EXT ".lib"
#endif
#pragma comment(lib, "tbb" TBB_LIB_EXT)
#pragma comment(lib, "tbbmalloc" TBB_LIB_EXT)
#endif

#include <regex>
#include <tbb/tbb.h>
#include "squish.h"
#include "lz4.h"
#include "lz4frame.h"
#include "lz4hc.h"

const char* kFormatStrings[] = {"DXT 1", "DXT 3", "DXT 5"};
static const int kGpuFmts[] = {squish::kDxt1, squish::kDxt3, squish::kDxt5};
static const int kGpuVideoFmts[] = {GPU_COMPRESS_DXT1, GPU_COMPRESS_DXT3, GPU_COMPRESS_DXT5};

//--------------------------------------------------------------
void ofApp::setup(){
    _imgui.setup();
    _gpuVideo.load("footage.gv", ofxExtremeGpuVideo::GPU_VIDEO_STREAMING_FROM_CPU_MEMORY_DECOMPRESSED);

#if ENABLE_BENCHMARK
    for(int i = 0 ; i < _videos.size() ; ++i) {
        _videos[i].load("footage.gv", ofxExtremeGpuVideo::GPU_VIDEO_STREAMING_FROM_STORAGE);
    }
#endif
}

//--------------------------------------------------------------
void ofApp::update(){
    _gpuVideo.setTime(_gpuVideo.getDuration() * ((float)ofGetMouseX() / ofGetWidth()));
    _gpuVideo.update();
    
#if ENABLE_BENCHMARK
    float e = ofGetElapsedTimef();
    for(int i = 0 ; i < _videos.size() ; ++i) {
        float t = e + i * 3.0f;
        _videos[i].setTime(fmodf(t, _videos[i].getDuration()));
    }
    
    tbb::parallel_for(tbb::blocked_range<int>( 0, _videos.size(), 1 ), [=](const tbb::blocked_range< int >& range) {
        for (int i = range.begin(); i != range.end(); i++) {
            _videos[i].updateCPU();
        }
    });
    for(int i = 0 ; i < _videos.size() ; ++i) {
        _videos[i].uploadGPU();
    }
#endif
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofClear(128);
    
    _gpuVideo.begin();
    _gpuVideo.getPlaceHolderTexture().draw(0, 0);
    _gpuVideo.end();

#if ENABLE_BENCHMARK
    float x = 0.0f;
    float y = 0.0f;
    float scale = 0.1f;
    for(int i = 0 ; i < _videos.size() ; ++i) {
        float w = _videos[i].getWidth() * scale;
        float h = _videos[i].getHeight() * scale;
        
        _videos[i].begin();
        _videos[i].getPlaceHolderTexture().draw(x, y, w, h);
        _videos[i].end();
        
        x += w;
        if(ofGetWidth() < x) {
            x = 0;
            y += h;
        }
    }
#endif

    if(_isConverting) {
        ofSetColor(255);
        if(_index < _imagePaths.size()) {
            auto compress = [=](int index, uint8_t *dst) {
                std::string src = _imagePaths[index];
                ofPixels img;
                ofLoadImage(img, src);
                img.setImageType(OF_IMAGE_COLOR_ALPHA);
                
                squish::CompressImage(img.getData(), _width, _height, dst, _squishFlag);
            };

            const int kBatchCount = 64;
            int workCount = std::min((int)_imagePaths.size() - _index, kBatchCount);
            
            uint32_t lz4sizes[kBatchCount];
            int compressBound = LZ4_compressBound(_bufferSize);
            
            _gpuCompressBuffer.resize(workCount * _bufferSize);
            _lz4CompressBuffer.resize(workCount * compressBound);
            
            tbb::parallel_for(tbb::blocked_range<int>( 0, workCount, 1 ), [=, &lz4sizes](const tbb::blocked_range< int >& range) {
                for (int i = range.begin(); i != range.end(); i++) {
                    compress(_index + i, _gpuCompressBuffer.data() + i * _bufferSize);
                    lz4sizes[i] = LZ4_compress_HC((char *)_gpuCompressBuffer.data() + i * _bufferSize,
                                                  (char *)_lz4CompressBuffer.data() + i * compressBound,
                                                  _bufferSize, compressBound, 16);
                    printf("%d -> %d, %.2f%%\n", (int)_bufferSize, lz4sizes[i], (float)lz4sizes[i] / (float)_bufferSize);
                }
            });
            
            uint32_t head = _lz4blocks.empty() ? kRawMemoryAt : (_lz4blocks[_lz4blocks.size() - 1].address + _lz4blocks[_lz4blocks.size() - 1].size);
            for (int i = 0; i < workCount; i++) {
                // 住所を記録しつつ
                Lz4Block lz4block;
                lz4block.address = head;
                lz4block.size = lz4sizes[i];
                head += lz4block.size;
                _lz4blocks.push_back(lz4block);
                
                // 書き込み
                if(fwrite(_lz4CompressBuffer.data() + i * compressBound, 1, lz4sizes[i], _fp) != lz4sizes[i]) {
                    assert(0);
                }
            }

            _index += workCount;
        } else {
            // 最後に住所を記録
            uint32_t size = _lz4blocks.size() * sizeof(Lz4Block);
            if(fwrite(_lz4blocks.data(), 1, size, _fp) != size) {
                assert(0);
            }
            
            // ファイルをクローズ
            fclose(_fp);
            _fp = nullptr;
            
            // 終了
            _isConverting = false;
        }
    }
    
    ofSetColor(255);

    _imgui.begin();
    ImGui::SetNextWindowPos(ofVec2f(10, 30), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ofVec2f(400, 600), ImGuiSetCond_Once);
    ImGui::Begin("Compression Setting");
    
    ImGui::Text("fps: %.2f", ofGetFrameRate());
    if(_isConverting == false) {
        int format = _format;
        ImGui::Combo("Format", &format, kFormatStrings, 3);
        _format = format;
        
        if(ImGui::Button("compress")) {
            this->startCompression();
        }
    } else {
        ImGui::Text("converting...(%d / %d)", _index, (int)_imagePaths.size());
    }
    
    ImGui::End();
    
    _imgui.end();
}
void ofApp::startCompression() {
    if(_isConverting) {
        return;
    }
    
    ofFileDialogResult r = ofSystemLoadDialog("select intermediate dir(.gvintermediate)", true);
    if(r.bSuccess == false) {
        return;
    }
    auto dstDefault = ofFile(r.getPath()).getBaseName() + ".gv";
    ofFileDialogResult sr = ofSystemSaveDialog(dstDefault.c_str(), "save to...");
    if(sr.bSuccess == false) {
        return;
    }
    
    _dstPath = sr.getPath();
    
    // 初期化
    _imagePaths.clear();
    _width = 0;
    _height = 0;
    _fps = 0;
    _index = 0;
    _lz4blocks.clear();
    
    // パス
    std::string root = r.getPath();
    
    for(int i = 0 ; true ; ++i) {
        char path[1024];
        sprintf(path, "%s/%05d.png", root.c_str(), i + 1);
        ofFile inputPath(path);
        if(inputPath.exists() == false) {
            break;
        }
        _imagePaths.push_back(path);
    }
    
    if(_imagePaths.size() > 0) {
        ofPixels f;
        ofLoadImage(f, _imagePaths[0]);
        _width = f.getWidth();
        _height = f.getHeight();
    } else {
        return;
    }
    
    // fps
    char metaPath[1024];
    sprintf(metaPath, "%s/meta.txt", root.c_str());
    std::ifstream ifs(metaPath);
    std::string meta((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    
    std::regex regex("Video:.*, ?([\\d\\.]+)\\s?fps");
    std::smatch match;
    
    _fps = 30.0f;
    if(std::regex_search(meta, match, regex)) {
        if(match.size() == 2) {
            _fps = ofToFloat(match[1]);
        }
    }
    
    _squishFlag = squish::kColourIterativeClusterFit | kGpuFmts[_format];
    // _squishFlag = squish::kColourRangeFit | squish::kColourMetricUniform | kGpuFmts[_format];
    _bufferSize = squish::GetStorageRequirements(_width, _height, _squishFlag);
    
    // 書き出し開始
    _fp = fopen(_dstPath.c_str(), "wb");
    
    // ヘッダー情報書き出し
#define W(v) if(fwrite(&v, 1, sizeof(v), _fp) != sizeof(v)) { assert(0); }
    W(_width);
    W(_height);
    
    uint32_t frameCount = (uint32_t)_imagePaths.size();
    W(frameCount);
    W(_fps);
    uint32_t videoFmt = kGpuVideoFmts[_format];
    W(videoFmt);
    W(_bufferSize);
#undef W
    
    _isConverting = true;
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}