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

inline ofPixels estimateAlphaZeroColor(const ofPixels &pixels) {
	if (pixels.getImageType() != OF_IMAGE_COLOR_ALPHA) {
		return pixels;
	}
	int w = pixels.getWidth();
	int h = pixels.getHeight();
	ofPixels dstPixels;
	dstPixels.allocate(w, h, OF_IMAGE_COLOR_ALPHA);
	uint8_t *dst = dstPixels.getData();
	const uint8_t *src = pixels.getData();

	struct coord {
		coord(int x, int y) :dx(x), dy(y) {
		}
		int dx; int dy;
	};

	std::vector<std::vector<coord>> samplecoords;

	for (int i = 0; i < 3; ++i) {
		int dx = 1;
		int dy = 0;

		int x = -1 * (1 + i);
		int y = -1 * (1 + i);

		std::vector<coord> cs;

		for (int k = 0; k < 4; ++k) {
			int nstep = 3 + 2 * i - 1;
			for (int j = 0; j < nstep; ++j) {
				cs.emplace_back(x, y);

				x += dx;
				y += dy;
			}

			int new_dx = -dy;
			int new_dy = dx;
			dx = new_dx;
			dy = new_dy;
		}

		samplecoords.push_back(cs);
	}

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			int index = (w * y + x) * 4;
			memcpy(dst + index, src + index, 4);

			if (src[index + 3] != 0) {
				memcpy(dst + index, src + index, 4);
				continue;
			}

			float rgb[3] = { 0.0f , 0.0f , 0.0f };
			int count = 0;

			for (int j = 0; j < samplecoords.size(); ++j) {
				for (auto c : samplecoords[j]) {
					int sx = x + c.dx;
					int sy = y + c.dy;
					if (sx < 0 || w <= sx) { continue; }
					if (sy < 0 || h <= sy) { continue; }

					int index_sample = (w * sy + sx) * 4;
					if (src[index_sample + 3] == 0) {
						continue;
					}
					for (int i = 0; i < 3; ++i) {
						rgb[i] += src[index_sample + i];
					}
					count++;
				}

				if (count != 0) {
					for (int i = 0; i < 3; ++i) {
						int comp = round(rgb[i] / count);
						comp = std::min(comp, 255);
						dst[index + i] = (uint8_t)comp;
					}
					dst[index + 3] = 0;
					break;
				}
			}

			if (count == 0) {
				memcpy(dst + index, src + index, 4);
			}
		}
	}
	return dstPixels;
}

template <class T>
void imgui_draw_tree_node(const char *name, bool isOpen, T f) {
    if(isOpen) {
        ImGui::SetNextTreeNodeOpened(true, ImGuiSetCond_Once);
    }
    if(ImGui::TreeNode(name)) {
        f();
        ImGui::TreePop();
    }
}

inline void images_to_gv(std::string output_path, std::vector<std::string> imagePaths, float fps, std::atomic<int> &done_frames, std::atomic<bool> &interrupt, bool liteMode, bool hasAlpha, bool isEstimateAlphaZeroColor) {
    if(imagePaths.empty()) {
        return;
    }
    
    // memory
    uint32_t _width = 0;
    uint32_t _height = 0;
    float _fps = fps;
    uint32_t _bufferSize = 0;
    uint32_t _squishFlag = 0;
    
    std::vector<uint8_t> _gpuCompressBuffer;
    std::vector<uint8_t> _lz4CompressBuffer;
    std::vector<Lz4Block> _lz4blocks;
    std::unique_ptr<GpuVideoIO> _io;
    
    int _index = 0;
    
    int width;
    int height;
    ofPixels img;
    ofLoadImage(img, imagePaths[0]);

    width = img.getWidth();
    height = img.getHeight();
    
    _width = width;
    _height = height;
    
    uint32_t flagQuality = liteMode ? (squish::kColourRangeFit | squish::kColourMetricUniform) : squish::kColourIterativeClusterFit;
    
    _squishFlag = flagQuality | (hasAlpha ? squish::kDxt5 : squish::kDxt1);
    _bufferSize = squish::GetStorageRequirements(_width, _height, _squishFlag);
    
    // 書き出し開始
    _io = std::unique_ptr<GpuVideoIO>(new GpuVideoIO(output_path.c_str(), "wb"));
    
    // ヘッダー情報書き出し
#define W(v) if(_io->write(&v, sizeof(v)) != sizeof(v)) { assert(0); }
    W(_width);
    W(_height);
    
    uint32_t frameCount = (uint32_t)imagePaths.size();
    W(frameCount);
    W(_fps);
    uint32_t videoFmt = hasAlpha ? GPU_COMPRESS_DXT5 : GPU_COMPRESS_DXT1;
    W(videoFmt);
    W(_bufferSize);
#undef W
    
    for(;;) {
        if(_index < imagePaths.size()) {
            auto compress = [imagePaths, _width, _height, _squishFlag, isEstimateAlphaZeroColor](int index, uint8_t *dst) {
                std::string src = imagePaths[index];
                
                ofPixels img;
                ofLoadImage(img, src);
                img.setImageType(OF_IMAGE_COLOR_ALPHA);

				if (isEstimateAlphaZeroColor) {
					img = estimateAlphaZeroColor(img);
				}
                
                squish::CompressImage(img.getData(), _width, _height, dst, _squishFlag);
            };
            
            const int kBatchCount = 32;
            int workCount = std::min((int)imagePaths.size() - _index, kBatchCount);
            
            uint32_t lz4sizes[kBatchCount];
            int compressBound = LZ4_compressBound(_bufferSize);
            
            _gpuCompressBuffer.resize(workCount * _bufferSize);
            _lz4CompressBuffer.resize(workCount * compressBound);
            
            tbb::parallel_for(tbb::blocked_range<int>( 0, workCount, 1 ), [compress, _index, _bufferSize, compressBound, &lz4sizes, &_gpuCompressBuffer, &_lz4CompressBuffer, &done_frames](const tbb::blocked_range< int >& range) {
                for (int i = range.begin(); i != range.end(); i++) {
                    compress(_index + i, _gpuCompressBuffer.data() + i * _bufferSize);
                    lz4sizes[i] = LZ4_compress_HC((char *)_gpuCompressBuffer.data() + i * _bufferSize,
                                                  (char *)_lz4CompressBuffer.data() + i * compressBound,
                                                  _bufferSize, compressBound, LZ4HC_CLEVEL_DEFAULT);
                    done_frames++;
                }
            });
            
            uint64_t head = _lz4blocks.empty() ? kRawMemoryAt : (_lz4blocks[_lz4blocks.size() - 1].address + _lz4blocks[_lz4blocks.size() - 1].size);
            for (int i = 0; i < workCount; i++) {
                // 住所を記録しつつ
                Lz4Block lz4block;
                lz4block.address = head;
                lz4block.size = lz4sizes[i];
                head += lz4block.size;
                _lz4blocks.push_back(lz4block);
                
                // 書き込み
                if(_io->write(_lz4CompressBuffer.data() + i * compressBound, lz4sizes[i]) != lz4sizes[i]) {
                    assert(0);
                }
            }
            
            _index += workCount;
            
            // 強制離脱
            if(interrupt) {
                _io.reset();
                ::remove(output_path.c_str());
                break;
            }
        } else {
            // 最後に住所を記録
            uint64_t size = _lz4blocks.size() * sizeof(Lz4Block);
            if(_io->write(_lz4blocks.data(), size) != size) {
                assert(0);
            }
            
            // ファイルをクローズ
            _io.reset();
            
            // 終了
            break;
        }
    }
}

template<typename R>
bool is_ready(std::future<R> const& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetVerticalSync(false);
    ofSetFrameRate(30);
    
    _imgui.setup();
    _abortTask = false;
}
void ofApp::exit() {
    _abortTask = true;
}

//--------------------------------------------------------------
void ofApp::update(){
    
}

void ofApp::startCompression() {
    _isConverting = true;
    _tasks.clear();
    
    for(auto input : _inputs) {
        std::shared_ptr<ConvTask> task = std::make_shared<ConvTask>();
        
        ofDirectory dir(input);
        dir.allowExt("png");
        dir.allowExt("jpeg");
        dir.allowExt("jpg");
        dir.allowExt("tiff");
        dir.allowExt("tif");
        dir.listDir();
        for(int i = 0 ; i < dir.size() ; ++i) {
            std::string path = dir.getPath(i);
			auto name = std::filesystem::path(path).filename().string();
			if (0 < name.size() && name[0] != '.') {
				task->image_paths.push_back(path);
			}
        }
		std::sort(task->image_paths.begin(), task->image_paths.end());

        task->output_path = input + ".gv";
        task->done_frames = 0;
        
        if(task->output_path.empty() == false) {
            _tasks.push_back(task);
        }
    }
}
//--------------------------------------------------------------
void ofApp::draw() {
    if(_abortTask) {
        return;
    }
    
    if(_isConverting) {
        bool all_done = true;
        for(int i = 0 ; i < _tasks.size() ; ++i) {
            std::shared_ptr<ConvTask> task = _tasks[i];
            if(task->done) {
                continue;
            }
            
            if(task->run) {
                if(is_ready(task->work)) {
                    task->work.get();
                    task->done = true;
                    continue;
                } else {
                    all_done = false;
                    break;
                }
            } else {
                task->run = true;
                auto output_path = task->output_path;
                auto image_paths = task->image_paths;
                auto fps = _fps;
                std::atomic<int> &done_frames = task->done_frames;
                std::atomic<bool> &abortTask = _abortTask;
                auto liteMode = _liteMode;
                auto hasAlpha = _hasAlpha;
				auto isEstimate = _isEstimateAlphaZeroColor;
                task->work = std::async([output_path, image_paths, fps, &done_frames, &abortTask, liteMode, hasAlpha, isEstimate](){
                    images_to_gv(output_path, image_paths, fps, done_frames, abortTask, liteMode, hasAlpha, isEstimate);
                    return 0;
                });
                all_done = false;
                break;
            }
        }
        
        if(all_done) {
            _dones = _inputs;
            _inputs.clear();
            _tasks.clear();
            _isConverting = false;
        }
    }
    
    ofClear(128);
    
    _imgui.begin();
    ImGui::SetNextWindowPos(ofVec2f(10, 30), ImGuiSetCond_Once);
    ImGui::SetNextWindowSize(ofVec2f(ofGetWidth() - 50, ofGetHeight() - 50), ImGuiSetCond_Once);
    ImGui::Begin("Compression");
    ImGui::Text("fps: %.2f", ofGetFrameRate());
    
    if(_isConverting == false) {
        imgui_draw_tree_node("Inputs (Please Drag and Drop)", true, [=]() {
            for(int i = 0 ; i < _inputs.size() ; ++i) {
                ImGui::Text("[%d]: %s", i, _inputs[i].c_str());
            }
        });
        imgui_draw_tree_node("Dones", true, [=]() {
            for(int i = 0 ; i < _dones.size() ; ++i) {
                ImGui::Text("[%d]: %s", i, _dones[i].c_str());
            }
        });
        if(_inputs.empty() == false) {
            if(ImGui::Button("Clear Input", ImVec2(200, 30))) {
                _inputs.clear();
            }
        }

        imgui_draw_tree_node("Option", true, [=]() {
            ImGui::Checkbox("Lite Mode", &_liteMode);
            ImGui::Checkbox("Has Alpha", &_hasAlpha);
            ImGui::InputFloat("video fps", &_fps);
            _fps = std::max(_fps, 1.0f);
            _fps = std::min(_fps, 3000.0f);
			ImGui::Checkbox("estimate alpha zero color", &_isEstimateAlphaZeroColor);
        });
        if(_inputs.empty() == false) {
            if(ImGui::Button("Run", ImVec2(200, 30))) {
                this->startCompression();
            }
        }
        
        //
    } else {
        imgui_draw_tree_node("Option", true, [=]() {
            ImGui::Text("Lite Mode: %s", _liteMode ? "YES" : "NO");
            ImGui::Text("Has Alpha: %s", _hasAlpha  ? "YES" : "NO");
        });
        imgui_draw_tree_node("Progress", true, [=]() {
            for(int i = 0 ; i < _tasks.size() ; ++i) {
                std::shared_ptr<ConvTask> task = _tasks[i];
                ImGui::Text("[%d]: %s (%d / %d)", i, task->output_path.c_str(), (int)task->done_frames.load(), (int)task->image_paths.size());
            }
        });
    }

    ImGui::End();
    
    _imgui.end();
    
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
void ofApp::dragEvent(ofDragInfo dragInfo) {
    // Only Dirs
    for(auto input : dragInfo.files) {
        ofDirectory dir(input);
        if(dir.isDirectory()) {
            _inputs.push_back(input);
        }
    }
    
    // dup check
    std::vector<std::string> unique_inputs;
    std::set<std::string> unique_set;
    for(auto input : _inputs) {
        if(unique_set.count(input) == 0) {
            unique_inputs.push_back(input);
            unique_set.insert(input);
        } else {
            // skip
        }
    }
    
    std::swap(_inputs, unique_inputs);
}
