#include <iostream>
#include <string>
#include <sstream>
#include <cassert>
#include "captureThread.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "pubDef.h"
#include "picQ.hpp"

using namespace std;
using namespace cv;

extern int getCurState(void);


string videoSrc;
bool fRunning=false;

const string wndNames[CAM_ST_NUM] = {
    "Body Detection Window"
    ,"Camera Moving Window"
    ,"Face Detection Window"
};

Rect wndRect[CAM_ST_NUM] = {
    Rect(20,400,480,320)
    ,Rect(620,400,480,320)
    ,Rect(1220,400,480,320)
};

int skipCnt[CAM_ST_NUM] = {
    0,0,0
};
int skipLimit[CAM_ST_NUM] = {
    1,1,1
};

int getSkipLimit(int kind) {
    if (kind < CAM_ST_NUM) {
        return skipLimit[kind];
    }
    else {
        return 0;
    }
}

void setSkipLimit(int kind, int limit) {
    if (kind < CAM_ST_NUM) {
        skipLimit[kind] = limit;
    }
}

void insertFrame2Q (mediaQueue<frame_t> *frmQ, Mat &picFrm, int frmCnt) {
    frame_t *frm = new frame_t;
    frm->frame = picFrm.clone();
    frm->frameCnt = frmCnt;
    frmQ->enq(frm);
}

bool isCapRunning(void) {
    return fRunning;
}

void capThread(capThrdCfg_t *pCtc) {
    cout << "capThread started" << endl;
    if ((nullptr == pCtc) 
        || (nullptr == pCtc->vidSrcStr)
        || (nullptr == pCtc->frmBodyDetQPtr)
        || (nullptr == pCtc->frmFaceDetQPtr)
        ) {
        stringstream err;
        err << "Invalid video source:" << endl;  
        throw err.str();
        return;
    }
    mediaQueue<frame_t> *frmQ[CAM_ST_NUM];
    frmQ[CAM_ST_OVERVIEW] = (mediaQueue<frame_t> *)(pCtc->frmBodyDetQPtr);
    frmQ[CAM_ST_MOVING] = nullptr;
    frmQ[CAM_ST_ZOMMIN] = (mediaQueue<frame_t> *)(pCtc->frmFaceDetQPtr);

    VideoCapture cap;
    Mat frame;
    string vidSrc(pCtc->vidSrcStr);
    cap.open(vidSrc);
    
    for (int i=0;i<CAM_ST_NUM;i++) {
        if (nullptr == frmQ[i]){
            namedWindow(wndNames[i], WINDOW_NORMAL);
            moveWindow(wndNames[i], wndRect[i].x, wndRect[i].y);
            resizeWindow(wndNames[i], wndRect[i].width, wndRect[i].height);
        }
    }
    int camSt=-1, camPrevSt=-1;
    int frameCnt=0;
    fRunning = true;
    do {
        cap >> frame;
         if (frame.empty()) {
            waitKey();
            break;
        }
        frameCnt++;
        camPrevSt = camSt;
        camSt = getCurState();
        if(camSt!=camPrevSt) {
            skipCnt[camSt] = 0;
        }
        if (++skipCnt[camSt] >= skipLimit[camSt]){
            skipCnt[camSt] = 0;
            string info = format("%d:%d/%d", frameCnt,skipCnt[camSt],skipLimit[camSt]);
            putText(frame, info.c_str(), Point(wndRect[camSt].width/2-100, wndRect[camSt].height/2+50), FONT_HERSHEY_SIMPLEX, 1, BGR_YELLOW, 2);
            if (frmQ[camSt]) {
                insertFrame2Q(frmQ[camSt], frame, frameCnt);
            }
            else {
                imshow(wndNames[camSt], frame);    
            }
        }
        else {
            string info = format("%d:%d/%d", frameCnt,skipCnt[camSt],skipLimit[camSt]);
            putText(frame, info.c_str(), Point(wndRect[camSt].width/2-100, wndRect[camSt].height/2+50), FONT_HERSHEY_SIMPLEX, 1, BGR_BLUE, 2);
            if (frmQ[camSt]) {
                insertFrame2Q(frmQ[camSt], frame, frameCnt);
            }
            else {
                imshow(wndNames[camSt], frame);    
            }
        }
    }while(waitKey(1) < 0);
    fRunning=false;
    for (int i=0;i<CAM_ST_NUM;i++) {
        /* send terminate signal to all threads */
        if (nullptr != frmQ[i]){
            insertFrame2Q(frmQ[i], frame, -1);
        }
    }
    cout << "capThread ended" << endl;
}