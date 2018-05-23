#ifndef __tracking_tool__Region__
#define __tracking_tool__Region__

#include "ofMain.h"
#include "ofxOsc.h"

class TTRegion {
    
public:
    TTRegion(int type);
    bool pointIsInZone(float x, float y);
    bool mousePressed(float x, float y);
    void mouseReleased();
    void mouseMoved(float x, float y);
    void render();

    // PARAMETERS
    ofVec2f position;
    ofVec2f size;
    int regionType;
    string oscString;

    // not used currently
    float xMinNormalized;
    float xMaxNormalized;
    float yMinNormalized;
    float yMaxNormalized;

    // STATUS (TO BE RENDERED)
    float status_activation;
    ofVec2f status_trackedPosition;
    bool armed;
    bool selected;

private:
    ofVec2f vertices[4];
    int selectedVertice;
    ofVec2f mouseClickedPosition;
    ofVec2f preMovedPosition;
    bool mouseOver;
    void recalculateSizeAndPosition();
    void recalculateVerticies();
};

#endif