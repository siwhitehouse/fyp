#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){
    
    
    //--------- set up kinect
	kinect.setRegistration(true); // enable depth->video image calibration
    
	kinect.init();
	kinect.open();
	
	if(kinect.isConnected()) {
		ofLogNotice() << "sensor-emitter dist: " << kinect.getSensorEmitterDistance() << "cm";
		ofLogNotice() << "sensor-camera dist:  " << kinect.getSensorCameraDistance() << "cm";
		ofLogNotice() << "zero plane pixel size: " << kinect.getZeroPlanePixelSize() << "mm";
		ofLogNotice() << "zero plane dist: " << kinect.getZeroPlaneDistance() << "mm";
        
        bUseKinect = true;
	}
    
    nearThreshold = 200;
	farThreshold = 20;
    angle = kinect.getTargetCameraTiltAngle();
    
    //--------- create image for holding masked rgb pixels
    int width = 640;
    int height = 480;
    colorImageMasked.allocate(width,height);
    depthImage.allocate(width,height);
    grayBackgroundCapture.allocate(width,height);
    bCaptureBg = false;
    
    //--------- set avatar
    totalAvatarsThisUser = 0;
    currentAvatar = -1;
    bRecordingAvatar = false;
    bSavingRecords = false;
    avatarOffX = ofGetWidth()*.5+200;

    recorder.setup(640,480);
    recorder.setFormat("png");

    for(int i = 0; i < MAX_AVATARS; i++){
        avatars[i].setup();
        float yp = (i+1) * (ofGetHeight()/4.0);
        avatars[i].pos.set(avatarOffX,yp);//xp,ofGetHeight()*.5);
    }
    
    
    //--------- set application settings
    ofSetFrameRate(30);
    
    //--------- gui
    bShowGui = true;

}

//--------------------------------------------------------------
void ofApp::update(){
    
    
    if(bUseKinect){
        
    //--------- update kinect
	kinect.update();
    
    //--------- create masked rgb frame
    if(kinect.isFrameNew()) {
		
		depthImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
		colorImageMasked.setFromPixels(kinect.getPixelsRef());
        if(bCaptureBg){
            grayBackgroundCapture.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
            bCaptureBg = false;
        }
        depthImage -= grayBackgroundCapture;
        depthImage.erode_3x3();
        depthImage.dilate_3x3();
        depthImage.dilate_3x3();
        depthImage.dilate_3x3();
        depthImage.blurGaussian();
        unsigned char * pixG = depthImage.getPixels();
        unsigned char * pixC = colorImageMasked.getPixels();
        
        int numPixels = depthImage.getWidth() * depthImage.getHeight();
        for(int i = 0; i < numPixels; i++) {
            if(pixG[i] < nearThreshold && pixG[i] > farThreshold) {
                pixG[i] = 255;
            } else {
                pixG[i] = 0;
                pixC[i*3] = 0;
                pixC[i*3+1] = 0;
                pixC[i*3+2] = 0;
            }
        }
    }
    
    colorImageMasked.flagImageChanged();
    depthImage.flagImageChanged();
    
    
    //--------- update recording
    // this is a hack to not have too many frames, should be frame rate based
    if(bRecordingAvatar && ofGetFrameNum() % 2 == 0){
        recorder.addFrame(colorImageMasked.getPixelsRef());
    }else if(bSavingRecords){
        
        if(recorder.q.size() == 0){
            bSavingRecords = false;
            cout << "done saving frames" << endl;
            // start new avatar
            if(!avatars[currentAvatar].isPlaying()){
                cout << "start avatar" << endl;
                avatars[currentAvatar].startAvatar();
            }
        }else{
            cout << " saving frames" << endl;
        }
    }
    
    }
    
    //--------- update avatar players
    if(totalAvatarsThisUser > 0){
        for( int i = 0; i < totalAvatarsThisUser; i++){
            avatars[i].update();
        }
    }
    
}
//--------------------------------------------------------------
void ofApp::startRecording(){
    
    if(!bUseKinect) return;
    
    if(totalAvatarsThisUser >= MAX_AVATARS || bSavingRecords){
        bRecordingAvatar = false;
        return;
    }
    
    cout << "start recording " << endl;
    
    currentAvatar++;
    totalAvatarsThisUser++;

    // ? reset avatar
    
    // set avatar directory
    string dir = "avatar_"+ofGetTimestampString();
    avatars[currentAvatar].setDirectory(dir);
    
    // start recorder
    recorder.startThread();
    string filePrefix = dir + "/frame_";
    recorder.setPrefix(ofToDataPath(filePrefix));

    // set is recording
    bRecordingAvatar = true;
    
    
}
//--------------------------------------------------------------
void ofApp::endRecording(){
    
    if(!bUseKinect) return;
    
    if(totalAvatarsThisUser >= MAX_AVATARS+1) return;

    cout << "end recording " << endl;
    bRecordingAvatar = false;
    bSavingRecords = true;
    recorder.stopThread();
    
}

void ofApp::exit(){
    if(bUseKinect) recorder.waitForThread();
    if(bUseKinect) kinect.close();
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofBackground(0);
    

    
    ofSetColor(255, 255, 255);
    //--------- draw avatar
    if(totalAvatarsThisUser > 0){
        for( int i = 0; i < totalAvatarsThisUser; i++){
            avatars[i].draw();
        }
    }
    
    if(bShowGui){
        
        if(bUseKinect){
            
            //--------- draw masked live color image
            float xp = 160*2+20*2;//(ofGetWidth() - colorImageMasked.getWidth() ) *.5;
            float yp = 20;//(ofGetHeight() - colorImageMasked.getHeight() ) *.5;
            colorImageMasked.draw(xp,yp,160,120);
            
            //--------- draw preview
            kinect.draw(20, 20, 160, 120);
            kinect.drawDepth(40+160, 20, 160, 120);
            
        }
        
        ofSetColor(255, 255, 255);
        stringstream reportStream;
        if(bUseKinect){
            
            reportStream << "set near threshold " << nearThreshold << " (press: + -)" << endl
            << "set far threshold " << farThreshold << endl << "fps: " << ofGetFrameRate() << endl <<
            "r - toggle recording" << endl << "f - toggle fullscreen" <<  endl << "g - toggle gui " << endl
            << "x - clear all avatars" << endl << "recording: " << bRecordingAvatar << endl;
            /*stringstream c;
            c << "Recording: " << bRecordingAvatar << "\nThread running: " << recorder.isThreadRunning() <<  "\nQueue Size: " << recorder.q.size() << "\n\nPress 'r' to toggle recording.\nPress 't' to toggle worker thread." << endl;
    
            ofDrawBitmapString(c.str(), 650, 50);*/
        }else{
            reportStream << "1,2,3 - Load prerecorded avatars.\n" << "g - toggle this text on/off\n"
            << "f - toggle fullscreen\n" <<  "fps: " << ofGetFrameRate() << endl;
            
        }
            ofDrawBitmapString(reportStream.str(), 650, 10);
    }
}

//--------------------------------------------------------------
void ofApp::setupGui(){
    
    // sliders for near and far planes of kinect
    // sliders for box center, w,h,d
    // toggle fullscreen
    // show fps
    
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    switch(key){
        case 'b':
            bCaptureBg = !bCaptureBg;
            break;
        case 'r':
            bRecordingAvatar = !bRecordingAvatar;
            if(bRecordingAvatar) startRecording();
            else endRecording();
            break;
        case '>':
		case '.':
			farThreshold ++;
			if (farThreshold > 255) farThreshold = 255;
			break;
			
		case '<':
		case ',':
			farThreshold --;
			if (farThreshold < 0) farThreshold = 0;
			break;
			
		case '+':
		case '=':
			nearThreshold ++;
			if (nearThreshold > 255) nearThreshold = 255;
			break;
			
		case '-':
			nearThreshold --;
			if (nearThreshold < 0) nearThreshold = 0;
			break;
        case OF_KEY_UP:
			angle++;
			if(angle>30) angle=30;
			if(bUseKinect) kinect.setCameraTiltAngle(angle);
			break;
			
		case OF_KEY_DOWN:
			angle--;
			if(angle<-30) angle=-30;
			if(bUseKinect) kinect.setCameraTiltAngle(angle);
			break;
        case OF_KEY_LEFT:
            avatarOffX+=5;
            for( int i = 0; i < totalAvatarsThisUser; i++){
                avatars[i].pos.x = avatarOffX;
            }
            break;
        case OF_KEY_RIGHT:
            avatarOffX-=5;
            for( int i = 0; i < totalAvatarsThisUser; i++){
                avatars[i].pos.x = avatarOffX;
            }
            break;
        case 'x':
            for(int i = 0; i < MAX_AVATARS; i++){
                avatars[i].resetAvatar();
            }
            totalAvatarsThisUser = 0;
            currentAvatar = -1;
            recorder.q.empty();
            break;
       case '1':
            if(!bUseKinect){
                //string myDir = openFile();
                
                avatars[totalAvatarsThisUser].setDirectory("avatar_2014-06-23-18-36-38-356");
                avatars[totalAvatarsThisUser].startAvatar();
                totalAvatarsThisUser++;
            }
            break;
        case '2':
            if(!bUseKinect){
                avatars[1].setDirectory("avatar_2014-06-23-18-37-35-194");
                avatars[1].startAvatar();
                totalAvatarsThisUser=2;
                
            }
            break;
        case '3':
            if(!bUseKinect){
                avatars[2].setDirectory("avatar_2014-06-23-18-36-23-113");
                avatars[2].startAvatar();
                totalAvatarsThisUser=3;
                
            }
            break;
        case 'f':
            ofToggleFullscreen();
            break;
        case 'g':
            bShowGui = !bShowGui;
            break;
            
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
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
