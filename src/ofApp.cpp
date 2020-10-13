#include "ofApp.h"

using namespace cv;
using namespace ofxCv;

//--------------------------------------------------------------
void ofApp::setup() {
    settings.loadFile("settings.xml");
    
    ofSetVerticalSync(false);
    ofHideCursor();

    appFramerate = settings.getValue("settings:app_framerate", 60);
    camFramerate = settings.getValue("settings:cam_framerate", 30);
    ofSetFrameRate(appFramerate);

    oscVideoQuality = settings.getValue("settings:osc_video_quality", 3); 
    videoColor = (bool) settings.getValue("settings:video_color", 0); 
    
    width = settings.getValue("settings:width", 640);
    height = settings.getValue("settings:height", 480);
    ofSetWindowShape(width, height);

    host = settings.getValue("settings:host", "127.0.0.1");
    oscHost = settings.getValue("settings:osc_host", "127.0.0.1");
    oscPort = settings.getValue("settings:osc_port", 7110);
    streamPort = settings.getValue("settings:stream_port", 7111);
    wsPort = settings.getValue("settings:ws_port", 7112);

    debug = (bool) settings.getValue("settings:debug", 1);
    rpiCamVersion = settings.getValue("settings:rpi_cam_version", 1);
    stillCompression = settings.getValue("settings:still_compression", 100);

    // camera
    if (videoColor) {
        gray.allocate(width, height, OF_IMAGE_COLOR);
    } else {
        gray.allocate(width, height, OF_IMAGE_GRAYSCALE);        
    }
    
    cam.setup(width, height, camFramerate, videoColor); // color/gray;

    camSharpness = settings.getValue("settings:sharpness", 0); 
    camContrast = settings.getValue("settings:contrast", 0); 
    camBrightness = settings.getValue("settings:brightness", 50); 
    camIso = settings.getValue("settings:iso", 300); 
    camExposureMode = settings.getValue("settings:exposure_mode", 0); 
    camExposureCompensation = settings.getValue("settings:exposure_compensation", 0); 
    camShutterSpeed = settings.getValue("settings:shutter_speed", 0);

    cam.setSharpness(camSharpness);
    cam.setContrast(camContrast);
    cam.setBrightness(camBrightness);
    cam.setISO(camIso);
    cam.setExposureMode((MMAL_PARAM_EXPOSUREMODE_T) camExposureMode);
    cam.setExposureCompensation(camExposureCompensation);
    cam.setShutterSpeed(camShutterSpeed);
    //cam.setFrameRate // not implemented in ofxCvPiCam
    
    // ~ ~ ~   get a persistent name for this computer   ~ ~ ~
    compname = "RPi";
    file.open(ofToDataPath("compname.txt"), ofFile::ReadWrite, false);
    ofBuffer buff;
    if (file) { // use existing file if it's there
        buff = file.readToBuffer();
        compname = buff.getText();
    } else { // otherwise make a new one
        compname += "_" + ofGetTimestampString("%y%m%d%H%M%S%i");
        ofStringReplace(compname, "\n", "");
        ofStringReplace(compname, "\r", "");
        buff.set(compname.c_str(), compname.size());
        ofBufferToFile("compname.txt", buff);
    }
   
    // * stream video *
    // https://github.com/bakercp/ofxHTTP/blob/master/libs/ofxHTTP/include/ofx/HTTP/IPVideoRoute.h
    // https://github.com/bakercp/ofxHTTP/blob/master/libs/ofxHTTP/src/IPVideoRoute.cpp
    streamSettings.setPort(streamPort);
    streamSettings.ipVideoRouteSettings.setMaxClientConnections(settings.getValue("settings:max_stream_connections", 5)); // default 5
    streamSettings.ipVideoRouteSettings.setMaxClientBitRate(settings.getValue("settings:max_stream_bitrate", 512)); // default 1024
    streamSettings.ipVideoRouteSettings.setMaxClientFrameRate(settings.getValue("settings:max_stream_framerate", 30)); // default 30
    streamSettings.ipVideoRouteSettings.setMaxClientQueueSize(settings.getValue("settings:max_stream_queue", 10)); // default 10
    streamSettings.ipVideoRouteSettings.setMaxStreamWidth(width); // default 1920
    streamSettings.ipVideoRouteSettings.setMaxStreamHeight(height); // default 1080
    streamSettings.fileSystemRouteSettings.setDefaultIndex("live_view.html");
    streamServer.setup(streamSettings);
    streamServer.start();

    fbo.allocate(width, height, GL_RGBA);
    pixels.allocate(width, height, OF_IMAGE_COLOR);

    ofSystem("cp /etc/hostname " + ofToDataPath("DocumentRoot/js/"));
    host = ofSystem("cat /etc/hostname");
    host.pop_back(); // last char is \n

    // * websockets *
    // https://github.com/bakercp/ofxHTTP/blob/master/libs/ofxHTTP/include/ofx/HTTP/WebSocketConnection.h
    // https://github.com/bakercp/ofxHTTP/blob/master/libs/ofxHTTP/src/WebSocketConnection.cpp
    wsSettings.setPort(wsPort);
    wsServer.setup(wsSettings);
    wsServer.webSocketRoute().registerWebSocketEvents(this);
    wsServer.start();

    // events: connect, open, close, idle, message, broadcast
    
    thresholdValue = settings.getValue("settings:threshold", 127); 
    
    debug = (bool) settings.getValue("settings:debug", 1);
    oscVideo = (bool) settings.getValue("settings:osc_video", 0); 
    blobs = (bool) settings.getValue("settings:blobs", 1);
    contours = (bool) settings.getValue("settings:contours", 0); 
    contourSlices = settings.getValue("settings:contour_slices", 10); 
    brightestPixel = (bool) settings.getValue("settings:brightest_pixel", 0); 

    contourThreshold = 2.0;
    contourMinAreaRadius = 1.0;
    contourMaxAreaRadius = 250.0;

    sender.setup(oscHost, oscPort);

    contourFinder.setMinAreaRadius(contourMinAreaRadius);
    contourFinder.setMaxAreaRadius(contourMaxAreaRadius);
    //contourFinder.setInvert(true); // find black instead of white
    trackingColorMode = TRACK_COLOR_RGB;
}

//--------------------------------------------------------------
void ofApp::update() {
     frame = cam.grab();

    if (!frame.empty()) {
        toOf(frame, gray.getPixelsRef());

        /*
        fbo.begin();
        gray.draw(0,0);
        fbo.end();
        
        fbo.readToPixels(pixels);
        */
        streamServer.send(gray.getPixels());
        
        if (oscVideo) {
            switch(oscVideoQuality) {
                case 5:
                    ofSaveImage(gray, videoBuffer, OF_IMAGE_FORMAT_JPEG, OF_IMAGE_QUALITY_BEST);
                    break;
                case 4:
                    ofSaveImage(gray, videoBuffer, OF_IMAGE_FORMAT_JPEG, OF_IMAGE_QUALITY_HIGH);
                    break;
                case 3:
                    ofSaveImage(gray, videoBuffer, OF_IMAGE_FORMAT_JPEG, OF_IMAGE_QUALITY_MEDIUM);
                    break;
                case 2:
                    ofSaveImage(gray, videoBuffer, OF_IMAGE_FORMAT_JPEG, OF_IMAGE_QUALITY_LOW);
                    break;
                case 1:
                    ofSaveImage(gray, videoBuffer, OF_IMAGE_FORMAT_JPEG, OF_IMAGE_QUALITY_WORST);
                    break;
            }
       	}
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(0);

    if(!frame.empty()) {
        if (debug) {
            if (!blobs && !contours) {
                drawMat(frame, 0, 0);
            } else if (blobs || contours) {
                drawMat(frameProcessed, 0, 0);
            }
        }

        if (oscVideo) {
            sendOscVideo();
        } 

        if (blobs) {
            if (debug) {
            	ofSetLineWidth(2);
            	ofNoFill();
            }
            
            //autothreshold(frameProcessed);        
            threshold(frame, frameProcessed, thresholdValue, 255, 0);
            contourFinder.setThreshold(contourThreshold);    
            contourFinder.findContours(frameProcessed);

            int n = contourFinder.size();
            for (int i = 0; i < n; i++) {
                float circleRadius;
                glm::vec2 circleCenter = toOf(contourFinder.getMinEnclosingCircle(i, circleRadius));
                if (debug) {
                	ofSetColor(cyanPrint);
                	ofDrawCircle(circleCenter, circleRadius);
                	ofDrawCircle(circleCenter, 1);
                }

                sendOscBlobs(i, circleCenter.x, circleCenter.y);
            }
        }

        if (contours) {
            if (debug) {
                ofSetLineWidth(2);
                ofNoFill();
            }

            int contourCounter = 0;
            unsigned char * pixels = gray.getPixels().getData();
            int gw = gray.getWidth();

            for (int h=0; h<255; h += int(255/contourSlices)) {
                contourFinder.setThreshold(h);
                contourFinder.findContours(frame);
                contourFinder.draw();            

                int n = contourFinder.size();
                for (int i = 0; i < n; i++) {
                    ofPolyline line = contourFinder.getPolyline(i);
                    vector<glm::vec3> cvPoints = line.getVertices();

                    int x = int(cvPoints[0].x);
                    int y = int(cvPoints[0].y);
                    ofColor col = pixels[x + y * gw];
                    float colorData[3]; 
                    colorData[0] = col.r;
                    colorData[1] = col.g;
                    colorData[2] = col.b;
                    char const * pColor = reinterpret_cast<char const *>(colorData);
                    std::string colorString(pColor, pColor + sizeof colorData);
                    contourColorBuffer.set(colorString); 

                    float pointsData[cvPoints.size() * 2]; 
                    for (int j=0; j<cvPoints.size(); j++) {
                        int index = j * 2;
                        pointsData[index] = cvPoints[j].x;
                        pointsData[index+1] = cvPoints[j].y;
                    }
                    char const * pPoints = reinterpret_cast<char const *>(pointsData);
                    std::string pointsString(pPoints, pPoints + sizeof pointsData);
                    contourPointsBuffer.set(pointsString); 

                    sendOscContours(contourCounter);
                    contourCounter++;
                }        
            }
        }
           
        if (brightestPixel) {
        	// this mostly useful as a performance baseline
            // https://openframeworks.cc/ofBook/chapters/image_processing_computer_vision.html
            float maxBrightness = 0; 
            float maxBrightnessX = 0; 
            float maxBrightnessY = 0;
            int skip = 2;

            for (int y=0; y<height - skip; y += skip) {
                for (int x=0; x<width - skip; x += skip) {
                    ofColor colorAtXY = gray.getColor(x, y);
                    float brightnessOfColorAtXY = colorAtXY.getBrightness();
                    if (brightnessOfColorAtXY > maxBrightness && brightnessOfColorAtXY > thresholdValue) {
                        maxBrightness = brightnessOfColorAtXY;
                        maxBrightnessX = x;
                        maxBrightnessY = y;
                    }
                }
            }

            if (debug) {
            	ofNoFill();
            	glm::vec2 circleCenter = glm::vec2(maxBrightnessX, maxBrightnessY);
            	ofDrawCircle(circleCenter, 40);
            }

            sendOscPixel(maxBrightnessX, maxBrightnessY);
        }
    }

    if (debug) {
        stringstream info;
        info << "FPS: " << ofGetFrameRate() << "\n";
        //info << "Camera Resolution: " << cam.width << "x" << cam.height << " @ "<< "xx" <<"FPS"<< "\n";
        ofDrawBitmapStringHighlight(info.str(), 10, 10, ofColor::black, ofColor::yellow);
    }
}

// ~ ~ ~ CAM ~ ~ ~
//void ofApp::onTakePhotoComplete(string fileName) {
    //ofLog() << "onTakePhotoComplete fileName: " << fileName;  

    //endTakePhoto(fileName);
//}

// ~ ~ ~ WEBSOCKETS ~ ~ ~
void ofApp::onWebSocketOpenEvent(ofxHTTP::WebSocketEventArgs& evt) {
    cout << "Websocket connection opened." << endl;// << evt.getConnectionRef().getClientAddress().toString() << endl;
}


void ofApp::onWebSocketCloseEvent(ofxHTTP::WebSocketCloseEventArgs& evt) {
    cout << "Websocket connection closed." << endl; //<< evt.getConnectionRef().getClientAddress().toString() << endl;
}


void ofApp::onWebSocketFrameReceivedEvent(ofxHTTP::WebSocketFrameEventArgs& evt) {
    cout << "Websocket frame was received:" << endl; // << evt.getConnectionRef().getClientAddress().toString() << endl;
    string msg = evt.frame().getText();
    cout <<  msg << endl;

    if (msg == "take_photo") {
        beginTakePhoto();
    }
    /*
    ofxJSONElement json;

    if (json.parse(evt.getFrameRef().getText())) {
        //std::cout << json.toStyledString() << std::endl;

        if (json.isMember("command") && json["command"] == "SET_BACKGROUND_COLOR") {
            if (json["data"] == "white") {
                //bgColor = ofColor::white;
            } else if (json["data"] == "black") {
                //bgColor = ofColor::black;
            } else {
                //cout << "Unknown color: " << json["data"].toStyledString() << endl;
            }
        }
    } else {
        //ofLogError("ofApp::onWebSocketFrameReceivedEvent") << "Unable to parse JSON: "  << evt.getFrameRef().getText();
    }
    */
}


void ofApp::onWebSocketFrameSentEvent(ofxHTTP::WebSocketFrameEventArgs& evt) {
    cout << "Websocket frame was sent." << endl;
}


void ofApp::onWebSocketErrorEvent(ofxHTTP::WebSocketErrorEventArgs& evt) {
    cout << "Websocket Error." << endl; //<< evt.getConnectionRef().getClientAddress().toString() << endl;
}

// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
void ofApp::createResultHtml(string fileName) {
    string photoIndexFileName = "DocumentRoot/result.html";
    ofBuffer buff;
    ofFile photoIndexFile;
    photoIndexFile.open(ofToDataPath(photoIndexFileName), ofFile::ReadWrite, false);

    string photoIndex = "<!DOCTYPE html>\n";
    
    if (fileName == "none") { // use existing file if it's there
        photoIndex += "<html><head><meta http-equiv=\"refresh\" content=\"0\"></head><body>\n";
        photoIndex += "WAIT\n";
    } else { // otherwise make a new one
        photoIndex += "<html><head></head><body>\n";
        
        string lastFile = ofFilePath::getFileName(fileName);

        //lastPhotoTakenName = ofFilePath::getFileName(fileName);
        lastPhotoTakenName = host + "_" + lastFile;
        ofSystem("mv " + ofToDataPath("DocumentRoot/photos/" + lastFile) + " " + ofToDataPath("DocumentRoot/photos/" + lastPhotoTakenName));
        
        photoIndex += "<a href=\"photos/" + lastPhotoTakenName + "\">" + lastPhotoTakenName + "</a>\n";
    }

    photoIndex += "</body></html>\n";

    buff.set(photoIndex.c_str(), photoIndex.size());
    ofBufferToFile(photoIndexFileName, buff);
}

void ofApp::beginTakePhoto() {
    //cam.takePhoto();
    createResultHtml("none");
}

void ofApp::endTakePhoto(string fileName) {
    createResultHtml(fileName);

    string msg = host + "," + lastPhotoTakenName;
    wsServer.webSocketRoute().broadcast(ofxHTTP::WebSocketFrame(msg));
}

void ofApp::sendOscVideo() {
    ofxOscMessage m;
    m.setAddress("/video");
    m.addStringArg(compname);    
    
    m.addBlobArg(videoBuffer);
    
    sender.sendMessage(m);
}

void ofApp::sendOscBlobs(int index, float x, float y) {
    ofxOscMessage m;
    m.setAddress("/blob");
    m.addStringArg(compname);
    
    m.addIntArg(index);
    m.addFloatArg(x / (float) width);
    m.addFloatArg(y / (float) height);

    sender.sendMessage(m);
}

void ofApp::sendOscContours(int index) {
    ofxOscMessage m;
    m.setAddress("/contour");
    m.addStringArg(compname);
    
    m.addIntArg(index);
    m.addBlobArg(contourColorBuffer);
    m.addBlobArg(contourPointsBuffer);

    sender.sendMessage(m);
}

void ofApp::sendOscPixel(float x, float y) {
    ofxOscMessage m;
    m.setAddress("/pixel");
    m.addStringArg(compname);
    
    m.addFloatArg(x / (float) width);
    m.addFloatArg(y / (float) height);

    sender.sendMessage(m);
}
