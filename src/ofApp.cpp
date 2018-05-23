#include "ofApp.h"

void ofApp::setup(){
    ofSetDataPathRoot("../Resources/");
    ofSetFrameRate(30);
            
    captureWidth = 640;
    captureHeight = 480;
    
    for (int i = 0; i < NR_OF_CAMERAS; i++) {
        grabbers[i].setDeviceID(i * 2);
        grabbers[i].initGrabber(captureWidth, captureHeight);
    }
    
    regionCounter = 0;
    
    
    fbo.allocate(captureWidth * NR_OF_CAMERAS, captureHeight, GL_RGB);
    colorImage.allocate(captureWidth * NR_OF_CAMERAS, captureHeight);
    grayImage.allocate(captureWidth * NR_OF_CAMERAS, captureHeight);
    grayBg.allocate(captureWidth * NR_OF_CAMERAS, captureHeight);
    grayDiff.allocate(captureWidth * NR_OF_CAMERAS, captureHeight);
    
    backgroundMode = 2;
    
    blendFactor = 0.3;
    threshold = 30;
    
    oscSender.setup("127.0.0.1", 57120);
    
    drawContourFinder = false;
    
    inputFlippedHorizontally = false;
    inputFlippedVertically = false;
}


void ofApp::update(){
    for (int i = 0; i < NR_OF_CAMERAS; i++) {
        grabbers[i].update();
    }
    
    ofPixels pixels;
    fbo.readToPixels(pixels);
    colorImage.setFromPixels(pixels);
    colorImage.mirror(inputFlippedVertically, inputFlippedHorizontally);
    grayImage = colorImage;
    
    ofPixels bg_pixels = grayBg.getPixels();
    ofPixels gI_pixels = grayImage.getPixels();
    
    for (int i = 0; i < ( (captureWidth * NR_OF_CAMERAS) * captureHeight); i++) {
        bg_pixels[i] = (bg_pixels[i] * (1.0 - blendFactor)) + (gI_pixels[i] * blendFactor);
    }
    
    grayBg.setFromPixels(bg_pixels);
    
    grayDiff.absDiff(grayBg, grayImage);
    grayDiff.threshold(threshold);
    contourFinder.findContours(grayDiff, 4, (captureWidth * captureHeight * NR_OF_CAMERAS) * 0.5, 50, false);
    
    
    for (TTRegion& r : regions) {
        if (r.regionType == 0) { // TRACK
            int blobsInRegion = 0;
            
            float biggestBlobSize = -1.f;
            int biggestBlobIndex = 0;
            
            for (int i = 0; i < contourFinder.nBlobs; i++) {
                float x, y;
                x = contourFinder.blobs[i].centroid.x / (captureWidth * NR_OF_CAMERAS);
                y = contourFinder.blobs[i].centroid.y / captureHeight;
                if (r.pointIsInZone(x, y)) {
                    blobsInRegion += 1;
                    float size = contourFinder.blobs[i].area;
                    if (size > biggestBlobSize) {
                        biggestBlobSize = size;
                        biggestBlobIndex = i;
                    }
                }
            }
            
            r.status_activation = 0.0;
            if (blobsInRegion > 0) {
                float x, y;
                float regionX, regionY;
                x = contourFinder.blobs[biggestBlobIndex].centroid.x / (captureWidth * NR_OF_CAMERAS);
                y = contourFinder.blobs[biggestBlobIndex].centroid.y / captureHeight;
                regionX = (x - (r.position.x - (r.size.x * 0.5))) / r.size.x;
                regionY = (y - (r.position.y - (r.size.y * 0.5))) / r.size.y;
                
                r.status_trackedPosition.x = x;
                r.status_trackedPosition.y = y;
                
                r.status_activation = 1;
                
                ofxOscMessage m;
                m.setAddress(r.oscString);
                m.addFloatArg(regionX);
                m.addFloatArg(regionY);
                oscSender.sendMessage(m);
            }
            
        } else if ((r.regionType == 1) || (r.regionType == 2)) {
            int pixelCounter = 0;
            int xStart, xEnd, yStart, yEnd;
            int xMin, xMax, yMin, yMax;
            
            xMin = 999999;
            xMax = -99999;
            yMin = 999999;
            yMax = -99999;
            
            xStart = int((r.position.x - (r.size.x * 0.5)) * (captureWidth * NR_OF_CAMERAS));
            xEnd = int((r.position.x + (r.size.x * 0.5)) * (captureWidth * NR_OF_CAMERAS));
            yStart = int((r.position.y - (r.size.y * 0.5)) * captureHeight);
            yEnd = int((r.position.y + (r.size.y * 0.5)) * captureHeight);
            
            xStart = CLAMP(xStart, 0, (captureWidth * NR_OF_CAMERAS));
            xEnd = CLAMP(xEnd, 0, (captureWidth * NR_OF_CAMERAS));
            yStart = CLAMP(yStart, 0, captureHeight);
            yEnd = CLAMP(yEnd, 0, captureHeight);
            
            for (int x = xStart; x < xEnd; x++) {
                for (int y = yStart; y < yEnd; y++) {
                    if (grayDiff.getPixels().getData()[(y * (captureWidth * NR_OF_CAMERAS)) + x] > 128) {
                        pixelCounter++;
                        if (r.regionType == 2) {
                            if (x < xMin) xMin = x;
                            if (x > xMax) xMax = x;
                            if (y < yMin) yMin = y;
                            if (y > yMax) yMax = y;
                        }
                    }
                }
            }
            
            if (r.regionType == 1) { // TRIGG
                r.status_activation = 0.0;
                if (pixelCounter > 1) {
                    if (r.armed) {
                        r.status_activation = 1.0;
                        ofxOscMessage m;
                        m.setAddress(r.oscString);
                        oscSender.sendMessage(m);
                        r.armed = false;
                    }
                } else {
                    r.armed = true;
                }
            }
            
            if (r.regionType == 2) { // ACTIVITY
                float xMinNormalized = float(xMin - xStart) / float(r.size.x * (captureWidth * NR_OF_CAMERAS));
                float xMaxNormalized = float(xMax - xStart) / float(r.size.x * (captureWidth * NR_OF_CAMERAS));
                float yMinNormalized = float(yMin - yStart) / float(r.size.y * captureHeight);
                float yMaxNormalized = float(yMax - yStart) / float(r.size.y * captureHeight);
                
                if ((xMinNormalized > 0.0) && (xMinNormalized < 1.0)) { r.xMinNormalized = xMinNormalized; }
                if ((xMaxNormalized > 0.0) && (xMaxNormalized < 1.0)) { r.xMaxNormalized = xMaxNormalized; }
                if ((yMinNormalized > 0.0) && (yMinNormalized < 1.0)) { r.yMinNormalized = yMinNormalized; }
                if ((yMaxNormalized > 0.0) && (yMaxNormalized < 1.0)) { r.yMaxNormalized = yMaxNormalized; }
                
                r.status_activation = float(pixelCounter) / ((r.size.x * (captureWidth * NR_OF_CAMERAS)) * (r.size.y * captureHeight));
                ofxOscMessage m;
                m.setAddress(r.oscString);
                m.addFloatArg(r.status_activation);
                m.addFloatArg(r.xMinNormalized);
                m.addFloatArg(r.xMaxNormalized);
                m.addFloatArg(r.yMinNormalized);
                m.addFloatArg(r.yMaxNormalized);
                oscSender.sendMessage(m);
            }
        }
    }
}

void ofApp::draw(){
    cout << grabbers[0].isFrameNew() << endl;
    
    fbo.begin();
    ofBackground(0);
    ofSetColor(255);
    ofSetRectMode(OF_RECTMODE_CORNER);
    for (int i = 0; i < NR_OF_CAMERAS; i++) {
        grabbers[i].draw(i * captureWidth, 0, captureWidth, captureHeight);
    }
    fbo.end();
    
    ofBackground(0);
    ofSetRectMode(OF_RECTMODE_CORNER);
    
    
    int lineHeight = 13;
    
    ofSetColor(255);
    switch (backgroundMode) {
        case 0:
            fbo.draw(0, 0, ofGetWidth(), ofGetHeight());
            ofDrawBitmapString("VIDEO GRABBER", 0, 0);
            break;
        case 1:
            colorImage.draw(0, 0, ofGetWidth(), ofGetHeight());
            ofDrawBitmapString("COLOR IMAGE", 0, 0);
            break;
        case 2:
            grayImage.draw(0, 0, ofGetWidth(), ofGetHeight());
            ofDrawBitmapString("GRAY IMAGE", 0, 0);
            break;
        case 3:
            grayBg.draw(0, 0, ofGetWidth(), ofGetHeight());
            ofDrawBitmapString("GRAY BG", 0, 0);
            break;
        case 4:
            grayDiff.draw(0, 0, ofGetWidth(), ofGetHeight());
            ofDrawBitmapString("GRAY DIFF", 0, 0);
            break;
    }
    ofPushMatrix();
    
    ofTranslate(6, 9);
    
    ofTranslate(0, lineHeight);
    ofTranslate(0, lineHeight);
    ofDrawBitmapString("BLEND FACTOR " + ofToString(blendFactor), 0, 0);
    ofTranslate(0, lineHeight);
    ofDrawBitmapString("THRESHOLD    " + ofToString(threshold), 0, 0);
    ofTranslate(0, lineHeight);
    ofDrawBitmapString("FPS          " + ofToString(ofGetFrameRate()), 0, 0);
    ofTranslate(0, lineHeight);
    ofDrawBitmapString("N BLOBS      " + ofToString(contourFinder.nBlobs), 0, 0);
    ofTranslate(0, lineHeight);
    ofDrawBitmapString("FLIPPED V    " + ofToString(inputFlippedVertically), 0, 0);
    ofTranslate(0, lineHeight);
    ofDrawBitmapString("FLIPPED H    " + ofToString(inputFlippedHorizontally), 0, 0);
    ofPopMatrix();
    
    if (drawContourFinder) {
        contourFinder.draw(0, 0, ofGetWidth(), ofGetHeight());
    } else {
        ofSetLineWidth(2);
        
        for (int i = 0; i < contourFinder.nBlobs; i++) {
            ofPushMatrix();
            ofSetColor(16, 232, 16, CLAMP(contourFinder.blobs[i].area, 64, 255));
            ofTranslate((contourFinder.blobs[i].centroid.x /  (captureWidth * NR_OF_CAMERAS)) * ofGetWidth(), (contourFinder.blobs[i].centroid.y / captureHeight) * ofGetHeight());
            ofDrawLine(-5, -5, 5, 5);
            ofDrawLine(-5, 5, 5, -5);
            ofPopMatrix();
        }
        ofSetLineWidth(1);
    }
    
    drawGrid();
    
    for (TTRegion r : regions) {
        r.render();
    }

    
}

void ofApp::keyPressed(int key){
    if (key == 'q') {
        grabbers[0].setDeviceID(0);
        grabbers[0].initGrabber(captureWidth, captureHeight);
    }
    if (key == 'w') {
        grabbers[0].setDeviceID(1);
        grabbers[0].initGrabber(captureWidth, captureHeight);
    }
    if (key == 'e') {
        grabbers[0].setDeviceID(2);
        grabbers[0].initGrabber(captureWidth, captureHeight);
    }

    if (key == 'r') {
        grabbers[1].setDeviceID(0);
        grabbers[1].initGrabber(captureWidth, captureHeight);
    }
    if (key == 't') {
        grabbers[1].setDeviceID(1);
        grabbers[1].initGrabber(captureWidth, captureHeight);
    }
    if (key == 'y') {
        grabbers[1].setDeviceID(2);
        grabbers[1].initGrabber(captureWidth, captureHeight);
    }
    
    if (key == OF_KEY_UP) {
        threshold++;
        if (threshold > 255) threshold = 255;
    }
    if (key == OF_KEY_DOWN) {
        threshold--;
        if (threshold < 0) threshold = 0;
    }
    if (key == OF_KEY_LEFT) {
        blendFactor *= 0.98;
        if (blendFactor < 0.01) blendFactor = 0.01;
    }
    if (key == OF_KEY_RIGHT) {
        blendFactor *= 1.02;
        if (blendFactor > 0.99) blendFactor = 0.99;
    }
    
    if (key == OF_KEY_BACKSPACE) {
        for (int i = 0; i < regions.size(); i++) {
            if (regions[i].selected) {
                regions.erase(regions.begin() + i);
            }
        }
    }
    
    if (key == '<') { backgroundMode = 0; }
    if (key == 'z') { backgroundMode = 1; }
    if (key == 'x') { backgroundMode = 2; }
    if (key == 'c') { backgroundMode = 3; }
    if (key == 'v') { backgroundMode = 4; }
    
    if (key == 'f') { drawContourFinder = !drawContourFinder; }
    
    if ((key == '1') || (key == '2') || (key == '3')) {
        if (key == '1') {
            regions.push_back(TTRegion(0));
        }
        if (key == '2') {
            regions.push_back(TTRegion(1));
        }
        if (key == '3') {
            regions.push_back(TTRegion(2));
        }
        regions[regions.size() - 1].oscString = "/r" + ofToString(regionCounter);
        regionCounter++;
    }
    
    if (key == 'o') {
        ofFileDialogResult openFileResult = ofSystemLoadDialog("Open save file (.xml)");
        readFile(openFileResult);
    }
    
    if (key == 's') {
        ofFileDialogResult saveFileResult = ofSystemSaveDialog(ofGetTimestampString() + ".xml", "Save your file!");
        saveFile(saveFileResult);
        
    }
    
    if (key == 'u') { inputFlippedHorizontally = !inputFlippedHorizontally; }
    if (key == 'i') { inputFlippedVertically = !inputFlippedVertically; }

}


void ofApp::mouseMoved(int x, int y){
    float normalizedX, normalizedY;
    normalizedX = float(x) / ofGetWidth();
    normalizedY = float(y) / ofGetHeight();
    for (int i = 0; i < regions.size(); i++) {
        regions[i].mouseMoved(normalizedX, normalizedY);
    }
}

void ofApp::mouseDragged(int x, int y, int button){
    float normalizedX, normalizedY;
    normalizedX = float(x) / ofGetWidth();
    normalizedY = float(y) / ofGetHeight();
    for (int i = 0; i < regions.size(); i++) {
        regions[i].mouseMoved(normalizedX, normalizedY);
    }
}

void ofApp::mousePressed(int x, int y, int button){
    float pressedX, pressedY;
    pressedX = float(x) / ofGetWidth();
    pressedY = float(y) / ofGetHeight();
    
    bool sentinel = false;
    for (int i = 0; i < regions.size(); i++) {
        if (sentinel == false) {
            sentinel = regions[i].mousePressed(pressedX, pressedY);
        }
    }
}

void ofApp::mouseReleased(int x, int y, int button){
    for (int i = 0; i < regions.size(); i++) {
        regions[i].mouseReleased();
    }
}

void ofApp::windowResized(int w, int h) { }

void ofApp::dragEvent(ofDragInfo dragInfo) { }

void ofApp::readFile(ofFileDialogResult openFileResult)
{
    regions.clear();
    
    ofxXmlSettings settings;
    if(settings.loadFile(openFileResult.getPath())){
        threshold = settings.getValue("threshold", 0.0);
        blendFactor = settings.getValue("blendFactor", 0.0);
        
        settings.pushTag("regions");
        int numberOfSavedRegions = settings.getNumTags("region");
        cout << "numberOfSavedRegions: " << numberOfSavedRegions << endl;
        for(int i = 0; i < numberOfSavedRegions; i++){
            settings.pushTag("region", i);
            
            TTRegion region(settings.getValue("regionType", 0));
            region.oscString = settings.getValue("oscString", "");
            region.position.x = settings.getValue("position-x", 0.0);
            region.position.y = settings.getValue("position-y", 0.0);
            region.size.x = settings.getValue("size-x", 0.0);
            region.size.y = settings.getValue("size-y", 0.0);
            
            regions.push_back(region);
            
            settings.popTag();
        }
        settings.popTag();
    }
}

void ofApp::saveFile(ofFileDialogResult saveFileResult) {
    
    ofxXmlSettings settings;
    
    settings.addValue("threshold", threshold);
    settings.addValue("blendFactor", blendFactor);
    
    settings.addTag("regions");
    settings.pushTag("regions");
    
    for(int i = 0; i < regions.size(); i++){
        settings.addTag("region");
        settings.pushTag("region", i);
        settings.addValue("regionType", regions[i].regionType);
        settings.addValue("oscString", regions[i].oscString);
        settings.addValue("position-x", regions[i].position.x);
        settings.addValue("position-y", regions[i].position.y);
        settings.addValue("size-x", regions[i].size.x);
        settings.addValue("size-y", regions[i].size.y);
        settings.popTag();
    }
    settings.popTag();
    settings.saveFile(saveFileResult.getPath());
}


void ofApp::drawGrid()
{
    ofSetColor(96);
    for (int r = 1; r < 8; r++) {
        float x = r * 0.125 * ofGetWidth();
        ofDrawLine(x, 0, x, ofGetHeight());
    }
    
    for (int c = 1; c < 4; c++) {
        float y = c * 0.25 * ofGetHeight();
        ofDrawLine(0, y, ofGetWidth(), y);
    }
}
