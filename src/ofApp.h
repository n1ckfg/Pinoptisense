#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxCvPiCam.h"
#include "ofxOsc.h"
#include "ofxXmlSettings.h"
#include "ofxHTTP.h"
#include "ofxJSONElement.h"

#define NUM_MESSAGES 30 // how many past ws messages we want to keep

class ofApp : public ofBaseApp {

    public:	
	void setup();
	void update();
	void draw();
		
	int width, height, appFramerate, camFramerate;
	
	string compname, host; // hostname;
	string oscHost;
	int oscPort, streamPort, wsPort;

	bool debug; // draw to local screen, default true

	ofFile file;
	ofxXmlSettings settings;

	ofFbo fbo;
	ofPixels pixels;
	int rpiCamVersion = 0; // 0 for not an RPi cam, 1 for v1, 2 for v2
	string lastPhotoTakenName;
	int stillCompression;

	void createResultHtml(string fileName);
	void beginTakePhoto();
	void endTakePhoto(string fileName);

	ofxHTTP::SimpleIPVideoServer streamServer;
	ofxHTTP::SimpleIPVideoServerSettings streamSettings;
	vector<string> photoFiles;
       
    ofxHTTP::SimpleWebSocketServer wsServer;  
	ofxHTTP::SimpleWebSocketServerSettings wsSettings;
	void onWebSocketOpenEvent(ofxHTTP::WebSocketEventArgs& evt);
	void onWebSocketCloseEvent(ofxHTTP::WebSocketCloseEventArgs& evt);
	void onWebSocketFrameReceivedEvent(ofxHTTP::WebSocketFrameEventArgs& evt);
	void onWebSocketFrameSentEvent(ofxHTTP::WebSocketFrameEventArgs& evt);
	void onWebSocketErrorEvent(ofxHTTP::WebSocketErrorEventArgs& evt);
    
	// ~ ~ ~ ~ ~ ~ ~     

	bool oscVideo; // send video image, default false
	bool brightestPixel; // send brightest pixel, default false
	bool blobs;  // send blob tracking, default true
	bool contours; // send contours, default false

	ofBuffer videoBuffer;
	ofBuffer contourColorBuffer;
	ofBuffer contourPointsBuffer;

	ofxCvPiCam cam;
	cv::Mat frame, frameProcessed;
	ofImage gray;
	int oscVideoQuality; // 5 best to 1 worst, default 3 medium
	bool videoColor;

	// for more camera settings, see:
	// https://github.com/orgicus/ofxCvPiCam/blob/master/example-ofxCvPiCam-allSettings/src/testApp.cpp

    int camShutterSpeed; // 0 to 330000 in microseconds, default 0
    int camSharpness; // -100 to 100, default 0
    int camContrast; // -100 to 100, default 0
    int camBrightness; // 0 to 100, default 50
	int camIso; // 100 to 800, default 300
	int camExposureCompensation; // -10 to 10, default 0;

	// 0 off, 1 auto, 2 night, 3 night preview, 4 backlight, 5 spotlight, 6 sports, 7, snow, 8 beach, 9 very long, 10 fixed fps, 11 antishake, 12 fireworks, 13 max
	int camExposureMode; // 0 to 13, default 0

	//string oscAddress;
	int thresholdValue; // default 127
	int thresholdKeyCounter;
	bool thresholdKeyFast;
	//bool doDrawInfo;

	ofxOscSender sender;
	void sendOscVideo();
	void sendOscBlobs(int index, float x, float y);
	void sendOscContours(int index);
	void sendOscPixel(float x, float y);

	ofxCv::ContourFinder contourFinder;
	float contourThreshold;  // default 127
	float contourMinAreaRadius; // default 10
	float contourMaxAreaRadius; // default 150
	int contourSlices; // default 20
	ofxCv::TrackingColorMode trackingColorMode; // RGB, HSV, H, HS; default RGB
	//ofColor targetColor; 

};
