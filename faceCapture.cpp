#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <X11/Xlib.h>
#include <chrono>
#include <thread>
#include <mutex>

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "pubDef.h"
#include "argParser.h"
#include "appInfo.h"

#include "picQ.hpp"
#include "postProcPic.hpp"
#include "faceDetThread.hpp"
#include "frmFeeder.h"
#include "frmFeeder.hpp"

using namespace std;
using namespace cv;
using namespace dnn;

static const std::string kWinName = "fitPose - Face Detetion";
static const std::string cWinName = "fitPose - Control Panel II";

faceDetThrd *ft1;

int ftAddToQ(frame_t *f) {
    int qSize;
    // tickZoomInTimer();
    qSize=ft1->AddToQ(f);
    //cout << "FaceDet:" << qSize << "," << f->frameCnt << endl;
    if(0==f->frameCnt) {
        cout << "Init NN processing for FaceDet..." << endl;
        while(ft1->isBusy()){
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
        }
    }
}

void getScreenResolution(int &width, int &height) {
    Display* disp = XOpenDisplay(NULL);
    Screen*  scrn = DefaultScreenOfDisplay(disp);
    width  = scrn->width;
    height = scrn->height;
    // cout << "wxh=" << width << "," << height << endl;
}

int main(int argc, char** argv)
{
    frmFeederParams_t ffp;
    /* model parameters for face detection */
    faceDetParams_t fdp;
    if(!parseFaceDetectArg(argc, argv, (void*)&ffp, fdp)) {
        exit(-1);
    }
    
    int scr_w, scr_h;
    int wndPosX,wndPosY;
    int wndWidth,wndHeight;
    getScreenResolution(scr_w, scr_h);
    wndPosX = scr_w/2;
    wndPosY = 2*WND_POSY_DFLT;
    wndHeight = scr_h/2;
    if (wndHeight < WND_HEIGHT_MIN) {
        wndHeight = WND_HEIGHT_MIN;
    }
    wndWidth = wndHeight * WND_WH_RATIO;
    if (wndWidth < WND_WIDTH_MIN) {
        wndWidth = WND_WIDTH_MIN;
    }

    ffp.func = (void *)ftAddToQ;
    frmFeederThrd *feeder = new frmFeederThrd(&ffp);

    frmProcParam_t FPP;
    FPP.wName = kWinName;
    FPP.wRect = Rect(wndPosX,wndPosY,wndWidth,wndHeight);

    namedWindow(FPP.wName, WINDOW_NORMAL);
    moveWindow(FPP.wName, FPP.wRect.x, FPP.wRect.y);
    resizeWindow(FPP.wName, FPP.wRect.width, FPP.wRect.height);

    string tName = "FaceDet";
    ft1 = new faceDetThrd(tName, (void *)&FPP, (void *)&fdp);
    cout << "Load NN:" << fdp.modelPath << fdp.configPath << endl;
    assert(ft1->loadNN(fdp.modelPath,fdp.configPath));
    assert(ft1->setNNCompTarget(fdp.compTarget));

    // setZoomInTimer(1);
    feeder->run();
    ft1->run(false);
    delete feeder;
    delete ft1;
    return 0;
}