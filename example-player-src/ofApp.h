#pragma once

#include "ofMain.h"
#include "ofxExtremeGpuVideo.hpp"

/* bench mark test */
#define ENABLE_BENCHMARK 0

/* MultiThread test */
#define CPU_PARALLEL_DECODE 0

class ofApp : public ofBaseApp {
public:
    void setup();
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
    
#if ENABLE_BENCHMARK
	std::array<ofxExtremeGpuVideo, 60> _videos;
#else
	ofxExtremeGpuVideo _gpuVideo;
#endif
};
