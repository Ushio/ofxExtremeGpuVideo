#include "ofApp.h"

bool hasEnding(std::string const &fullString, std::string const &ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
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
//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);
	_imgui.setup();
}

//--------------------------------------------------------------
void ofApp::update() {
	double at = ofGetElapsedTimeMicros() * 0.001 * 0.001;
	if (_previous < 0) {
		_previous = at;
		return;
	}
	double deltaT = at - _previous;
	_previous = at;

	if (_video.isLoaded()) {
		
		if (_play)
		{
			_elapsed += deltaT * _timescale;
		}

		if (_loop) {
			if (_video.getDuration() <= _elapsed) {
				_elapsed = 0.0;
			}
		}

		_elapsed = std::min(_elapsed, (double)_video.getDuration());
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	static bool isWhite = false;
	ofClear(isWhite ? 255 : 0);

	if (_video.isLoaded()) {
		_video.setTime(_elapsed);
		_video.update();

		if (_drawMode == DrawMode_DotByDot) {
			_video.getTexture().draw(0, 0);
		}
		else if (_drawMode == DrawMode_Fit) {
			ofRectangle dst_rect(0, 0, _video.getWidth(), _video.getHeight());
			dst_rect.scaleTo(ofRectangle(0, 0, ofGetWidth(), ofGetHeight()), OF_SCALEMODE_FIT);
			_video.getTexture().draw(dst_rect);
		}
		else if (_drawMode == DrawMode_Fill) {
			ofRectangle dst_rect(0, 0, _video.getWidth(), _video.getHeight());
			dst_rect.scaleTo(ofRectangle(0, 0, ofGetWidth(), ofGetHeight()), OF_SCALEMODE_FILL);
			_video.getTexture().draw(dst_rect);
		}
	}

	if (_showGui) {
		_imgui.begin();
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ofVec4f(0.0f, 0.2f, 0.2f, 1.0f));
		ImGui::SetNextWindowPos(ofVec2f(10, 30), ImGuiSetCond_Once);
		ImGui::SetNextWindowSize(ofVec2f(500, 600), ImGuiSetCond_Once);

		ImGui::Begin("Config Panel");
		ImGui::Text("Key bindings");
		ImGui::Text("f    : toggle fullscreen");
		ImGui::Text("h    : hide panel");
		ImGui::Text("space: play pause");
		ImGui::Text("right: next frame");
		ImGui::Text("left : prev frame");
		ImGui::Separator();
		ImGui::Text("fps: %.2f", ofGetFrameRate());

		ImGui::Checkbox("background white", &isWhite);

		if (_video.isLoaded()) {
			imgui_draw_tree_node("video info", true, [=]() {
				char videoPath[1024];
				sprintf(videoPath, "%s", _videoPath.c_str());
				ImGui::InputText("Video Path", videoPath, sizeof(videoPath), ImGuiInputTextFlags_ReadOnly);

				ImGui::Text("size : %d x %d", (int)_video.getWidth(), (int)_video.getHeight());
				ImGui::Text("fps : %.3f", _video.getFramePerSecond());
				ImGui::Text("duration : %.3fs", _video.getDuration());
				ImGui::Text("frames : %d", _video.getFrameCount());
			});

			imgui_draw_tree_node("playing", true, [=]() {
				ImGui::Checkbox("play", &_play);
				ImGui::Checkbox("loop", &_loop);

				imgui_draw_tree_node("speed", false, [=]() {
					ImGui::SliderFloat("timescale", &_timescale, 0.0f, 5.0f);
					if (ImGui::Button("x 1")) {
						_timescale = 1.0f;
					}
					if (ImGui::Button("x 0.5")) {
						_timescale = 0.5f;
					}
					if (ImGui::Button("x 2")) {
						_timescale = 2.0f;
					}
				});

				float at = _elapsed;
				if (ImGui::SliderFloat("time at", &at, 0.0f, _video.getDuration())) {
					_elapsed = at;
					_play = false;
				}

				int frameAt = _video.getFrameAt();
				if (ImGui::InputInt("frame at", &frameAt)) {
					if (0 <= frameAt && frameAt < _video.getFrameCount()) {
						double step = 1.0 / _video.getFramePerSecond();
						_elapsed = step * 0.5 + step * frameAt;
						_play = false;
					}
				}
			});
			imgui_draw_tree_node("drawing", true, [=]() {
				const char *items[] = {"Dot by dot", "Fit", "Fill"};
				ImGui::Combo("Draw Mode", &_drawMode, items, 3);
			});
		}
		else {
			ImGui::Text("please drag and drop");
		}
		
		if (ImGui::Button("open ...", ImVec2(150, 25))) {
			auto r = ofSystemLoadDialog("open gv");
			if (r.bSuccess) {
				if (hasEnding(r.filePath, ".gv")) {
					loadGV(r.filePath);
				}
			}
		}

		ImGui::End();
		ImGui::PopStyleColor();

		_imgui.end();

		ofShowCursor();
	}
	else {
		ofHideCursor();
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == ' ') {
		_play = !_play;
	}
	if (key == 'h') {
		_showGui = !_showGui;
	}
	if (key == 'f') {
		ofToggleFullscreen();
	}
	
	if (_video.isLoaded()) {
		int frameAt = _video.getFrameAt();

		if (key == OF_KEY_RIGHT) {
			frameAt++;
			if (frameAt < _video.getFrameCount()) {
				double step = 1.0 / _video.getFramePerSecond();
				_elapsed = step * 0.5 + step * frameAt;
				_play = false;
			}
		}
		if (key == OF_KEY_LEFT) {
			frameAt--;
			if (0 <= frameAt) {
				double step = 1.0 / _video.getFramePerSecond();
				_elapsed = step * 0.5 + step * frameAt;
				_play = false;
			}
		}
	}
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
void ofApp::dragEvent(ofDragInfo dragInfo){ 
	for (auto file : dragInfo.files) {
		if (hasEnding(file, ".gv")) {
			loadGV(file);
			break;
		}
	}
}
void ofApp::loadGV(std::string path) {
	_video.load(path, ofxGvTexture::GPU_VIDEO_STREAMING_FROM_CPU_MEMORY);
	_videoPath = path;
	_play = true;
	_elapsed = 0;
}