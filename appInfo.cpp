#include <string>
#include <sstream>
#include <iostream>

#include "appInfo.h"

using namespace std;

string faInfo;
int zoomInTimer, zoomInLimit;
string faceAreaInfo;

int getZoomInTimer(void) {
    return zoomInTimer;
}

bool setZoomInTimer(int z){
    zoomInTimer = z;
    return true;
}

int getZoomInLimit(void) {
    return zoomInLimit;
}

bool setZoomInLimit(int z){
    zoomInLimit = z;
    return true;
}

bool tickZoomInTimer(){
    zoomInTimer--;
    return true;
}
/*
float get(void){

}

bool set(float ){


}
*/

char const * getFAInfo (void) {
    return faInfo.c_str();
}

bool setFAInfo(char const *str ){
    faInfo = str;
    return true;
}

char const * getFaceAreaInfo (void) {
    return faceAreaInfo.c_str();
}

bool setFaceAreaInfo(char const *str ){
    faceAreaInfo = str;
    return true;
}