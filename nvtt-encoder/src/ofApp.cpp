#include "ofApp.h"

#include <locale> 

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
	if (isOpen) {
		ImGui::SetNextTreeNodeOpened(true, ImGuiSetCond_Once);
	}
	if (ImGui::TreeNode(name)) {
		f();
		ImGui::TreePop();
	}
}
inline void images_to_gv(std::string output_path, std::vector<std::string> imagePaths, float fps, int *done_frames, bool hasAlpha, bool isEstimateAlphaZeroColor, nvtt::Compressor &compressor, ofxCoroutine::Yield &yield) {
	if (imagePaths.empty()) {
		return;
	}

	// memory
	uint32_t _width = 0;
	uint32_t _height = 0;
	float _fps = fps;
	uint32_t _bufferSize = 0;

	
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

	auto setup = [=](bool hasAlpha, nvtt::InputOptions &inputOptions, nvtt::CompressionOptions &compOptions, nvtt::OutputOptions &outputOptions) {
		inputOptions.setTextureLayout(nvtt::TextureType_2D, _width, _height);
		inputOptions.setMipmapGeneration(false);
		inputOptions.setRoundMode(nvtt::RoundMode_None);
		inputOptions.setAlphaMode(hasAlpha ? nvtt::AlphaMode_Transparency : nvtt::AlphaMode_None);
		inputOptions.setFormat(nvtt::InputFormat_BGRA_8UB);

		compOptions.setFormat(hasAlpha ? nvtt::Format_DXT5 : nvtt::Format_DXT1);
		compOptions.setQuality(nvtt::Quality_Production);

		outputOptions.setOutputHeader(false);
	};
	nvtt::InputOptions inputOptions;
	nvtt::CompressionOptions compOptions;
	nvtt::OutputOptions outputOptions;
	setup(hasAlpha, inputOptions, compOptions, outputOptions);
	std::vector<uint8_t> emptydata(_width * _height * 4);
	inputOptions.setMipmapData(emptydata.data(), _width, _height);

	_bufferSize = compressor.estimateSize(inputOptions, compOptions);

	yield();

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

	struct Writer : public nvtt::OutputHandler {
		Writer(uint8_t *ptr) :_ptr(ptr) {}
		void beginImage(int size, int width, int height, int depth, int face, int miplevel) override {

		}
		bool writeData(const void * data, int size) override {
			memcpy(_ptr, data, size);
			_ptr += size;
			return true;
		}
		uint8_t *_ptr = nullptr;
	};
	struct ErrorHandler : public nvtt::ErrorHandler {
		void error(nvtt::Error e) {
			printf("%s\n", nvtt::errorString(e));
		}
	};

	int compressBound = LZ4_compressBound(_bufferSize);
	
	for (int i = 0; i < imagePaths.size(); ++i) {
		int beg = i;
		int batch = 1;
		while (i + 1 < imagePaths.size() && batch < 16) {
			batch++;
			i++;
		}

		#pragma omp parallel for ordered num_threads(16)
		for (int ompi = 0; ompi < batch; ++ompi)
		{
			int j = beg + ompi;

			nvtt::InputOptions inputOptions;
			nvtt::CompressionOptions compOptions;
			nvtt::OutputOptions outputOptions;
			setup(hasAlpha, inputOptions, compOptions, outputOptions);

			static thread_local ofPixels inputImage;
			ofLoadImage(inputImage, imagePaths[j]);
			inputImage.setImageType(OF_IMAGE_COLOR_ALPHA);

			if (isEstimateAlphaZeroColor) {
				inputImage = estimateAlphaZeroColor(inputImage);
			}

			inputImage.swapRgb();

			inputOptions.setMipmapData(inputImage.getPixels(), inputImage.getWidth(), inputImage.getHeight());

			ErrorHandler errorHandler;

			static thread_local std::vector<uint8_t> dxtBuffer;
			dxtBuffer.resize(_bufferSize);
			Writer writer(dxtBuffer.data());

			outputOptions.setOutputHandler(&writer);
			outputOptions.setErrorHandler(&errorHandler);

			bool s = compressor.process(inputOptions, compOptions, outputOptions);

			static thread_local std::vector<uint8_t> lz4CompressBuffer;
			lz4CompressBuffer.resize(compressBound);
			int compressed = LZ4_compress_HC((char *)dxtBuffer.data(),
				(char *)lz4CompressBuffer.data(),
				_bufferSize, compressBound, LZ4HC_CLEVEL_DEFAULT);

			#pragma omp ordered
			{
				// 住所を記録しつつ
				uint64_t head = _lz4blocks.empty() ? kRawMemoryAt : (_lz4blocks[_lz4blocks.size() - 1].address + _lz4blocks[_lz4blocks.size() - 1].size);
				Lz4Block lz4block;
				lz4block.address = head;
				lz4block.size = compressed;

				_lz4blocks.push_back(lz4block);

				// 書き込み
				if (_io->write(lz4CompressBuffer.data(), compressed) != compressed) {
					assert(0);
				}
			}
		}

		// (*done_frames)++;
		(*done_frames) += batch;

		yield();
	}

	// 最後に住所を記録
	uint64_t size = _lz4blocks.size() * sizeof(Lz4Block);
	if (_io->write(_lz4blocks.data(), size) != size) {
		abort();
	}

	// ファイルをクローズ
	_io.reset();
}

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(false);
	ofSetFrameRate(60);

	_compressor.enableCudaAcceleration(false);

	_imgui.setup();
}
void ofApp::exit() {
}
//--------------------------------------------------------------
void ofApp::update(){
	_coroutine.step();
}

void ofApp::startCompression() {
	_isConverting = true;
	_tasks.clear();

	for (auto input : _inputs) {
		std::shared_ptr<ConvTask> task = std::make_shared<ConvTask>();

		ofDirectory dir(input);
		dir.allowExt("png");
		dir.allowExt("jpeg");
		dir.allowExt("jpg");
		dir.allowExt("tiff");
		dir.allowExt("tif");
		dir.allowExt("PNG");
		dir.allowExt("JPEG");
		dir.allowExt("JPG");
		dir.allowExt("TIFF");
		dir.allowExt("TIF");
		dir.listDir();
		for (int i = 0; i < dir.size(); ++i) {
			std::string path = dir.getPath(i);
			auto name = std::filesystem::path(path).filename().string();
			if (0 < name.size() && name[0] != '.') {
				task->image_paths.push_back(path);
			}
		}
		std::sort(task->image_paths.begin(), task->image_paths.end());

		task->output_path = input + ".gv";
		task->done_frames = 0;

		if (task->output_path.empty() == false) {
			_tasks.push_back(task);
		}
	}

	_coroutine.add([=](ofxCoroutine::Yield &yield) {
		yield();

		for (int i = 0; i < _tasks.size(); ++i) {
			auto task = _tasks[i];
			images_to_gv(task->output_path, task->image_paths, _fps, &task->done_frames, _hasAlpha, _estimateAlphaZeroColor, _compressor, yield);
		}

		_dones = _inputs;
		_inputs.clear();
		_tasks.clear();
		_isConverting = false;
	}, 1024 * 1024 * 4);
}
//--------------------------------------------------------------
void ofApp::draw(){

	ofClear(128);

	_imgui.begin();
	ImGui::SetNextWindowPos(ofVec2f(10, 30), ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(ofVec2f(ofGetWidth() - 50, ofGetHeight() - 50), ImGuiSetCond_Once);
	ImGui::Begin("Compression");
	ImGui::Text("fps: %.2f", ofGetFrameRate());

	if (_isConverting == false) {
		imgui_draw_tree_node("Inputs (Please Drag and Drop)", true, [=]() {
			for (int i = 0; i < _inputs.size(); ++i) {
				ImGui::Text("[%d]: %s", i, _inputs[i].c_str());
			}
		});
		imgui_draw_tree_node("Dones", true, [=]() {
			for (int i = 0; i < _dones.size(); ++i) {
				ImGui::Text("[%d]: %s", i, _dones[i].c_str());
			}
		});
		if (_inputs.empty() == false) {
			if (ImGui::Button("Clear Input", ImVec2(200, 30))) {
				_inputs.clear();
			}
		}

		imgui_draw_tree_node("Option", true, [=]() {
			ImGui::Checkbox("Has Alpha", &_hasAlpha);
			ImGui::InputFloat("video fps", &_fps);
			_fps = std::max(_fps, 1.0f);
			_fps = std::min(_fps, 3000.0f);
			ImGui::Checkbox("estimate alpha zero color", &_estimateAlphaZeroColor);
		});
		if (_inputs.empty() == false) {
			if (ImGui::Button("Run", ImVec2(200, 30))) {
				this->startCompression();
			}
		}
	}
	else {
		imgui_draw_tree_node("Option", true, [=]() {
			ImGui::Text("Has Alpha: %s", _hasAlpha ? "YES" : "NO");
		});
		imgui_draw_tree_node("Progress", true, [=]() {
			for (int i = 0; i < _tasks.size(); ++i) {
				std::shared_ptr<ConvTask> task = _tasks[i];
				ImGui::Text("[%d]: %s (%d / %d)", i, task->output_path.c_str(), task->done_frames, (int)task->image_paths.size());
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
void ofApp::mouseMoved(int x, int y ){

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
	for (auto input : dragInfo.files) {
		ofDirectory dir(input);
		if (dir.isDirectory()) {
			_inputs.push_back(input);
		}
	}

	// dup check
	std::vector<std::string> unique_inputs;
	std::set<std::string> unique_set;
	for (auto input : _inputs) {
		if (unique_set.count(input) == 0) {
			unique_inputs.push_back(input);
			unique_set.insert(input);
		}
		else {
			// skip
		}
	}

	std::swap(_inputs, unique_inputs);
}