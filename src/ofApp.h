#pragma once

#include "ofMain.h"
#include "Region.h"
#include "ofxOpenCv.h"
#include "ofxOsc.h"
#include "ofxXmlSettings.h"


#define NR_OF_CAMERAS 1

class ofApp : public ofBaseApp{
    
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);

    ofTrueTypeFont font;
private:    
    void readFile(ofFileDialogResult openFileResult);
    void saveFile(ofFileDialogResult saveFileResult);

    ofVideoGrabber grabbers[NR_OF_CAMERAS];
    int captureWidth, captureHeight;
    ofFbo fbo;
    
    ofxCvColorImage colorImage;
    ofxCvGrayscaleImage grayImage;
    ofxCvGrayscaleImage grayBg;
    ofxCvGrayscaleImage grayDiff;
    int backgroundMode;
    bool inputFlippedHorizontally;
    bool inputFlippedVertically;
    
    ofxCvContourFinder contourFinder;
    
    float blendFactor;
    int threshold;
    
    void drawGrid();
    bool drawContourFinder;
    
    vector<TTRegion> regions;
    
    int regionCounter;
    
    ofxOscSender oscSender;
};
