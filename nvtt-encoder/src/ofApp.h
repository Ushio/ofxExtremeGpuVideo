#pragma once

#include "ofMain.h"

#include <memory>
#include <array>

#include "ofxImGui.h"

#define USE_COROUTINE_V2
#include "ofxCoroutine.hpp"

#include "GPUVideo.hpp"
#include "GpuVideoIO.hpp"

// https://developer.nvidia.com/gameworksdownload#?dn=gpu-accelerated-texture-tools-2-08
#include <nvtt/nvtt.h>
#include <nvtt/nvtt_wrapper.h>

class ofApp : public ofBaseApp{
public:
	void setup();
	void exit();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
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
	ofxCoroutine _coroutine;

	bool _isConverting = false;

	bool _hasAlpha = false;
	bool _estimateAlphaZeroColor = false;
	float _fps = 30.0f;
	std::vector<std::string> _inputs;
	std::vector<std::string> _dones;

	// タスク
	struct ConvTask {
		bool run = false;
		bool done = false;

		std::string output_path;
		std::vector<std::string> image_paths;
		int done_frames = 0;
	};
	std::vector<std::shared_ptr<ConvTask>> _tasks;
	nvtt::Compressor _compressor;
};
