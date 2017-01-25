#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "ofxGvTexture.hpp"

enum DrawMode {
	DrawMode_DotByDot = 0,
	DrawMode_Fit,
	DrawMode_Fill
};

class ofApp : public ofBaseApp{
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

	void loadGV(std::string path);

	double _previous = -1;

	ofxImGui _imgui;
	ofxGvTexture _video;

	std::string _videoPath;
	double _elapsed = 0.0f;

	bool _showGui = true;
	bool _play = false;
	bool _loop = true;

	float _timescale = 1.0f;

	int _drawMode = DrawMode_Fit;
};
