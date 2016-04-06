#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    _gpuVideo.load("footage.gv", ofxExtrimeGpuVideo::GPU_VIDEO_STREAMING_FROM_STORAGE);
}

//--------------------------------------------------------------
void ofApp::update(){
    _gpuVideo.setTime(_gpuVideo.getDuration() * ((float)ofGetMouseX() / ofGetWidth()));
    _gpuVideo.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofClear(128);
    
    _gpuVideo.begin();
    _gpuVideo.getPlaceHolderTexture().draw(0, 0);
    _gpuVideo.end();
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
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
