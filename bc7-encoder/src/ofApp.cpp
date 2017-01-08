#include "ofApp.h"

#include <locale> 
#include <codecvt>
#include <DirectXTex/DirectXTex/DirectXTex.h>

#pragma comment(lib,"d3d11.lib")

#include "lz4.h"
#include "lz4frame.h"
#include "lz4hc.h"

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
inline std::wstring to_wstring(std::string s) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
	return cv.from_bytes(s);
}
class Dx11 {
public:
	Dx11() {
		HRESULT hr;
		// D3D_FEATURE_LEVEL
		std::array<D3D_FEATURE_LEVEL, 1> FeatureLevel = {
			D3D_FEATURE_LEVEL_11_0
		};

#if defined(DEBUG) || defined(_DEBUG)
		UINT createDeviceFlag = D3D11_CREATE_DEVICE_DEBUG;
#else
		UINT createDeviceFlag = 0;
#endif
		D3D_FEATURE_LEVEL selected;

		hr = D3D11CreateDevice(
			nullptr,                  // 使用するアダプターを設定。NULLの場合はデフォルトのアダプター。
			D3D_DRIVER_TYPE_HARDWARE,    // D3D_DRIVER_TYPEのいずれか。ドライバーの種類。pAdapterが NULL 以外の場合は、D3D_DRIVER_TYPE_UNKNOWNを指定する。
			NULL,                       // ソフトウェアラスタライザを実装するDLLへのハンドル。D3D_DRIVER_TYPE を D3D_DRIVER_TYPE_SOFTWARE に設定している場合は NULL にできない。
			createDeviceFlag,           // D3D11_CREATE_DEVICE_FLAGの組み合わせ。デバイスを作成時に使用されるパラメータ。
			FeatureLevel.data(),               // D3D_FEATURE_LEVELのポインタ
			FeatureLevel.size(),                 // D3D_FEATURE_LEVEL配列の要素数
			D3D11_SDK_VERSION,          // DirectX SDKのバージョン。この値は固定。
			&d3d11device,               // 初期化されたデバイス
			&selected,              // 採用されたフィーチャーレベル
			&d3d11deviceContext         // 初期化されたデバイスコンテキスト
		);
		if (FAILED(hr)) {
			printf("failed to initialize dx11");
		}
	}
	~Dx11() {
		if (d3d11deviceContext) {
			d3d11deviceContext->Release();
			d3d11deviceContext = nullptr;
		}
		if (d3d11device) {
			d3d11device->Release();
			d3d11device = nullptr;
		}
	}
	Dx11(const Dx11 &) = delete;
	void operator=(const Dx11 &) = delete;

	ID3D11Device *device() {
		return d3d11device;
	}
private:
	ID3D11Device *d3d11device = nullptr;
	ID3D11DeviceContext* d3d11deviceContext = nullptr;
};

static std::shared_ptr<Dx11> dx11;

inline void images_to_gv(std::string output_path, std::vector<std::string> imagePaths, float fps, int *done_frames, bool hasAlpha, std::shared_ptr<Dx11> dx, ofxCoroutine::Yield &yield) {
	if (imagePaths.empty()) {
		return;
	}

	// memory
	uint32_t _width = 0;
	uint32_t _height = 0;
	float _fps = fps;
	uint32_t _bufferSize = 0;

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

	int blockcount = ((_width + 3) / 4) * ((_height + 3) / 4);
	int blocksize = 16;
	_bufferSize = blockcount * blocksize;

	// 書き出し開始
	_io = std::unique_ptr<GpuVideoIO>(new GpuVideoIO(output_path.c_str(), "wb"));

	// ヘッダー情報書き出し
#define W(v) if(_io->write(&v, sizeof(v)) != sizeof(v)) { assert(0); }
	W(_width);
	W(_height);

	uint32_t frameCount = (uint32_t)imagePaths.size();
	W(frameCount);
	W(_fps);
	uint32_t videoFmt = GPU_COMPRESS_BC7;
	W(videoFmt);
	W(_bufferSize);
#undef W

	int compressBound = LZ4_compressBound(_bufferSize);
	_lz4CompressBuffer.resize(compressBound);

	for (int i = 0; i < imagePaths.size(); ++i) {
		DirectX::TexMetadata metadata;
		DirectX::ScratchImage image;

		auto imgPath = ofToDataPath(imagePaths[i]);
		HRESULT hr = DirectX::LoadFromWICFile(to_wstring(imgPath).c_str(), 0, &metadata, image);
		if (FAILED(hr)) {
			abort();
		}


		DWORD flags = DirectX::TEX_COMPRESS_DEFAULT;
		flags |= DirectX::TEX_COMPRESS_PARALLEL;
		flags |= DirectX::TEX_COMPRESS_BC7_USE_3SUBSETS;
		flags |= DirectX::TEX_COMPRESS_UNIFORM;

		flags |= DirectX::TEX_COMPRESS_SRGB_IN;
		flags |= DirectX::TEX_COMPRESS_SRGB_OUT;

		float alphaWeight = hasAlpha ? 1.0f : 0.0f;
		DirectX::ScratchImage cImage;
		hr = DirectX::Compress(dx11->device(), *image.GetImage(0, 0, 0), DXGI_FORMAT_BC7_UNORM_SRGB, flags, alphaWeight, cImage);

		int src = cImage.GetPixelsSize();
		if (_bufferSize != cImage.GetPixelsSize()) {
			abort();
		}
		int compressed = LZ4_compress_HC((char *)cImage.GetPixels(),
						(char *)_lz4CompressBuffer.data(),
						_bufferSize, compressBound, 16);

		// 住所を記録しつつ
		uint64_t head = _lz4blocks.empty() ? kRawMemoryAt : (_lz4blocks[_lz4blocks.size() - 1].address + _lz4blocks[_lz4blocks.size() - 1].size);
		Lz4Block lz4block;
		lz4block.address = head;
		lz4block.size = compressed;
		_lz4blocks.push_back(lz4block);

		// 書き込み
		if (_io->write(_lz4CompressBuffer.data(), compressed) != compressed) {
			assert(0);
		}

		(*done_frames)++;

		yield();
	}

	// 最後に住所を記録
	uint64_t size = _lz4blocks.size() * sizeof(Lz4Block);
	if (_io->write(_lz4blocks.data(), size) != size) {
		abort();
	}

	// ファイルをクローズ
	_io.reset();
	
	//for (;;) {
	//	if (_index < imagePaths.size()) {
	//		auto compress = [imagePaths, _width, _height, _squishFlag](int index, uint8_t *dst) {
	//			std::string src = imagePaths[index];

	//			ofPixels img;
	//			ofLoadImage(img, src);
	//			img.setImageType(OF_IMAGE_COLOR_ALPHA);

	//			squish::CompressImage(img.getData(), _width, _height, dst, _squishFlag);
	//		};

	//		const int kBatchCount = 32;
	//		int workCount = std::min((int)imagePaths.size() - _index, kBatchCount);

	//		uint32_t lz4sizes[kBatchCount];
	//		int compressBound = LZ4_compressBound(_bufferSize);

	//		_gpuCompressBuffer.resize(workCount * _bufferSize);
	//		_lz4CompressBuffer.resize(workCount * compressBound);

	//		tbb::parallel_for(tbb::blocked_range<int>(0, workCount, 1), [compress, _index, _bufferSize, compressBound, &lz4sizes, &_gpuCompressBuffer, &_lz4CompressBuffer, &done_frames](const tbb::blocked_range< int >& range) {
	//			for (int i = range.begin(); i != range.end(); i++) {
	//				compress(_index + i, _gpuCompressBuffer.data() + i * _bufferSize);
	//				lz4sizes[i] = LZ4_compress_HC((char *)_gpuCompressBuffer.data() + i * _bufferSize,
	//					(char *)_lz4CompressBuffer.data() + i * compressBound,
	//					_bufferSize, compressBound, 16);

	//				done_frames++;
	//			}
	//		});

	//		uint64_t head = _lz4blocks.empty() ? kRawMemoryAt : (_lz4blocks[_lz4blocks.size() - 1].address + _lz4blocks[_lz4blocks.size() - 1].size);
	//		for (int i = 0; i < workCount; i++) {
	//			// 住所を記録しつつ
	//			Lz4Block lz4block;
	//			lz4block.address = head;
	//			lz4block.size = lz4sizes[i];
	//			head += lz4block.size;
	//			_lz4blocks.push_back(lz4block);

	//			// 書き込み
	//			if (_io->write(_lz4CompressBuffer.data() + i * compressBound, lz4sizes[i]) != lz4sizes[i]) {
	//				assert(0);
	//			}
	//		}

	//		_index += workCount;

	//		// 強制離脱
	//		if (interrupt) {
	//			_io.reset();
	//			::remove(output_path.c_str());
	//			break;
	//		}
	//	}
	//	else {
	//		// 最後に住所を記録
	//		uint64_t size = _lz4blocks.size() * sizeof(Lz4Block);
	//		if (_io->write(_lz4blocks.data(), size) != size) {
	//			assert(0);
	//		}

	//		// ファイルをクローズ
	//		_io.reset();

	//		// 終了
	//		break;
	//	}
	//}
}

//--------------------------------------------------------------
void ofApp::setup() {
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	dx11 = std::shared_ptr<Dx11>(new Dx11());

	ofSetVerticalSync(false);
	ofSetFrameRate(30);

	_imgui.setup();
}
void ofApp::exit() {
	dx11 = std::shared_ptr<Dx11>();
}
//--------------------------------------------------------------
void ofApp::update(){
	_coroutine.step();

	//if (_isConverting) {
	//	bool all_done = true;
	//	for (int i = 0; i < _tasks.size(); ++i) {
	//		std::shared_ptr<ConvTask> task = _tasks[i];
	//		if (task->done) {
	//			continue;
	//		}

	//		if (task->run) {
	//			if (is_ready(task->work)) {
	//				task->work.get();
	//				task->done = true;
	//				continue;
	//			}
	//			else {
	//				all_done = false;
	//				break;
	//			}
	//		}
	//		else {
	//			task->run = true;
	//			auto output_path = task->output_path;
	//			auto image_paths = task->image_paths;
	//			auto fps = _fps;
	//			std::atomic<int> &done_frames = task->done_frames;
	//			std::atomic<bool> &abortTask = _abortTask;
	//			auto liteMode = _liteMode;
	//			auto hasAlpha = _hasAlpha;
	//			task->work = std::async([output_path, image_paths, fps, &done_frames, &abortTask, liteMode, hasAlpha]() {
	//				images_to_gv(output_path, image_paths, fps, done_frames, abortTask, liteMode, hasAlpha);
	//				return 0;
	//			});
	//			all_done = false;
	//			break;
	//		}
	//	}

	//	if (all_done) {
	//		_dones = _inputs;
	//		_inputs.clear();
	//		_tasks.clear();
	//		_isConverting = false;
	//	}
	//}
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

	// _isConverting
	_coroutine.add([=](ofxCoroutine::Yield &yield) {
		yield();

		for (int i = 0; i < _tasks.size(); ++i) {
			auto task = _tasks[i];
			images_to_gv(task->output_path, task->image_paths, _fps, &task->done_frames, _hasAlpha, dx11, yield);
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
		});
		if (_inputs.empty() == false) {
			if (ImGui::Button("Run", ImVec2(200, 30))) {
				this->startCompression();
			}
		}

		//
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