#include "ofApp.h"

#if CPU_PARALLEL_DECODE
#include <tbb/tbb.h>

#ifdef _MSC_VER
#ifdef _DEBUG
#define TBB_LIB_EXT "_debug.lib"
#else
#define TBB_LIB_EXT ".lib"
#endif
#pragma comment(lib, "tbb" TBB_LIB_EXT)
#pragma comment(lib, "tbbmalloc" TBB_LIB_EXT)
#endif

#endif
//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);
#if ENABLE_BENCHMARK
	for (int i = 0; i < _videos.size(); ++i) {
		_videos[i].load("../../../resources/Atoms-8579.gv", ofxExtremeGpuVideo::GPU_VIDEO_STREAMING_FROM_STORAGE);
	}
#else
	_gpuVideo.load("../../../resources/Atoms-8579.gv", ofxExtremeGpuVideo::GPU_VIDEO_STREAMING_FROM_STORAGE);
	_gv.load("../../../resources/Atoms-8579.gv", ofxGvTexture::GPU_VIDEO_STREAMING_FROM_STORAGE);
#endif
}

//--------------------------------------------------------------
void ofApp::update(){
#if ENABLE_BENCHMARK
	// GetTime
	float e = ofGetElapsedTimef();

#if CPU_PARALLEL_DECODE
	// Advanced Multithread Update
	for (int i = 0; i < _videos.size(); ++i) {
		float t = e + i * 3.0f;
		_videos[i].setTime(fmodf(t, _videos[i].getDuration()));
	}
	tbb::parallel_for(tbb::blocked_range<int>(0, _videos.size(), 1), [=](const tbb::blocked_range< int >& range) {
		for (int i = range.begin(); i != range.end(); i++) {
			_videos[i].updateCPU();
		}
	});
	for (int i = 0; i < _videos.size(); ++i) {
		_videos[i].uploadGPU();
	}
#else
	// Simple Update
	for (int i = 0; i < _videos.size(); ++i) {
		float t = e + i * 3.0f;
		_videos[i].setTime(fmodf(t, _videos[i].getDuration()));
		_videos[i].update();
	}
#endif

#else
	float e = ofGetElapsedTimef();
	
	_gpuVideo.setTime(_gpuVideo.getDuration() * ((float)ofGetMouseX() / ofGetWidth()));
	//_gpuVideo.setTime(fmodf(e, _gpuVideo.getDuration()));
	_gpuVideo.update();

	_gv.setTime(_gpuVideo.getDuration() * ((float)ofGetMouseX() / ofGetWidth()));
	//_gv.setTime(fmodf(e, _gv.getDuration()));
	_gv.update();
#endif
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofClear((sin(ofGetElapsedTimef()) * 0.5 + 0.5) * 255);
#if ENABLE_BENCHMARK
	float x = 0.0f;
	float y = 0.0f;
	float scale = 0.1f;
	for (int i = 0; i < _videos.size(); ++i) {
		float w = _videos[i].getWidth() * scale;
		float h = _videos[i].getHeight() * scale;

		_videos[i].begin();
		_videos[i].getPlaceHolderTexture().draw(x, y, w, h);
		_videos[i].end();

		x += w;
		if (ofGetWidth() < x) {
			x = 0;
			y += h;
		}
	}
#else
	//_gpuVideo.begin();
	//_gpuVideo.getPlaceHolderTexture().draw(0, 0);
	//_gpuVideo.end();
	_gv.getTexture().draw(0, 0);
#endif

	ofDrawBitmapString(ofToString(ofGetFrameRate()), 10, 20);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == ' ') {
		ofToggleFullscreen();
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

}
