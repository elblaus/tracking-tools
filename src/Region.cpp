#include "Region.h"

TTRegion::TTRegion(int type)
{
    selectedVertice = -1;
    recalculateVerticies();
    selected = false;
    mouseOver = false;
    
    position = ofVec2f(0.5, 0.5);
    size = ofVec2f(0.25, 0.25);
    regionType = type;
    
    status_activation = 0;
    status_trackedPosition = ofVec2f(0, 0);
    armed = true;
    
}

void TTRegion::render()
{    
    ofPushMatrix();
    
    ofTranslate(position.x * ofGetWidth(), position.y * ofGetHeight());
    ofSetRectMode(OF_RECTMODE_CENTER);

    // BASIC REGION
    if (mouseOver) {
        ofSetColor(128, 128);
    } else {
        ofSetColor(0, 32);
    }
    if (selected) {
        ofSetColor(128, 128);
    }

    ofFill();
    ofSetLineWidth(1);
    ofDrawRectangle(0, 0, size.x * ofGetWidth(), size.y * ofGetHeight());
    
    
    ofSetColor(255);
    if (selected) {
        ofSetColor(255, 255, 128);
    }
    
    ofNoFill();
    ofDrawRectangle(0, 0, size.x * ofGetWidth(), size.y * ofGetHeight());

    ofDrawLine(0, -9, 0, 9);
    ofDrawLine(-9, 0, 9, 0);

    if ((regionType > 0) && (status_activation > 0.01)) {
        ofFill();
        ofSetColor(255, 255, 128, 255 * status_activation);
        ofDrawRectangle(0, 0, size.x * ofGetWidth(), size.y * ofGetHeight());
    }
    
    if ((regionType > 0) && (status_activation > 0.9)) {
        ofSetColor(0, 255);
    } else {
        ofSetColor(255, 255);
    }
    
    string str = "";
    if (mouseOver) {
        if (regionType == 0) str = "TRACKING";
        if (regionType == 1) str = "TRIGGER";
        if (regionType == 2) str = "ACTIVITY";
    } else {
        if (regionType == 0) str = "TRA";
        if (regionType == 1) str = "TRI";
        if (regionType == 2) str = "ACT";
    }
    ofDrawBitmapString(str, 4 - (size.x * ofGetWidth() * 0.5), 12 - (size.y * ofGetHeight() * 0.5));
    ofDrawBitmapString(oscString, 4 - (size.x * ofGetWidth() * 0.5), 22 - (size.y * ofGetHeight() * 0.5));
    ofDrawBitmapString(ofToString(  int(status_activation * 100) / 100.0  ), 4 - (size.x * ofGetWidth() * 0.5), 32 - (size.y * ofGetHeight() * 0.5));

    ofPopMatrix();

    if (regionType == 0) {
        if (status_activation > 0) {
            ofPushMatrix();
            ofTranslate(status_trackedPosition.x * ofGetWidth(), status_trackedPosition.y * ofGetHeight());
            ofNoFill();
            ofSetColor(255, 255, 128);
            ofDrawRectangle(0, 0, 8, 8);
            ofDrawLine(4, 0, 16, 0);
            ofDrawLine(-4, 0, -16, 0);
            ofDrawLine(0, 4, 0, 16);
            ofDrawLine(0, -4, 0, -16);
            ofPopMatrix();
            ofFill();
        }
    }
}

void TTRegion::mouseMoved(float x, float y)
{
    if (selected) {
        position = preMovedPosition + (ofVec2f(x, y) - mouseClickedPosition);
        recalculateVerticies();
    } else {
        mouseOver = pointIsInZone(x, y);
        
        if (selectedVertice > -1) {
            vertices[selectedVertice] = preMovedPosition + (ofVec2f(x, y) - mouseClickedPosition);
            if (selectedVertice == 0) {
                vertices[1].y = vertices[0].y;
                vertices[3].x = vertices[0].x;
            }
            if (selectedVertice == 1) {
                vertices[0].y = vertices[1].y;
                vertices[2].x = vertices[1].x;
            }
            if (selectedVertice == 2) {
                vertices[1].x = vertices[2].x;
                vertices[3].y = vertices[2].y;
            }
            if (selectedVertice == 3) {
                vertices[0].x = vertices[3].x;
                vertices[2].y = vertices[3].y;
            }
            recalculateSizeAndPosition();
        }
    }
}

bool TTRegion::pointIsInZone(float x, float y)
{
    bool sentinel = true;
    if (x < (position.x - (size.x * 0.5))) { sentinel = false; }
    if (x > (position.x + (size.x * 0.5))) { sentinel = false; }
    if (y < (position.y - (size.y * 0.5))) { sentinel = false; }
    if (y > (position.y + (size.y * 0.5))) { sentinel = false; }
    return sentinel;
}

bool TTRegion::mousePressed(float x, float y)
{
    bool clicked = false;
    
    for (int i = 0; i < 4; i++) {
        if (ofDist(x, y, vertices[i].x, vertices[i].y) < 0.01) {
            selectedVertice = i;
            mouseClickedPosition = ofVec2f(x, y);
            preMovedPosition = vertices[i];
            
            if (clicked == false) {
                clicked = true;
            }
        }
    }
    
    if (mouseOver && !clicked) {
        bool sentinel = true;
        
        if (sentinel) {
            selected = true;
            mouseClickedPosition = ofVec2f(x, y);
            preMovedPosition = position;
        }
        
        if (clicked == false) {
            clicked = true;
        }
    }
    
    return clicked;
}

void TTRegion::mouseReleased()
{
    selected = false;
    selectedVertice = -1;
}

void TTRegion::recalculateSizeAndPosition()
{
    position.x = (vertices[0].x + vertices[1].x) * 0.5;
    position.y = (vertices[0].y + vertices[2].y) * 0.5;
    
    size.x = (vertices[1].x - vertices[0].x);
    size.y = (vertices[2].y - vertices[0].y);
}

void TTRegion::recalculateVerticies()
{
    vertices[0] = ofVec2f(position.x - (size.x * 0.5), position.y - (size.y * 0.5));
    vertices[1] = ofVec2f(position.x + (size.x * 0.5), position.y - (size.y * 0.5));
    vertices[2] = ofVec2f(position.x + (size.x * 0.5), position.y + (size.y * 0.5));
    vertices[3] = ofVec2f(position.x - (size.x * 0.5), position.y + (size.y * 0.5));
}