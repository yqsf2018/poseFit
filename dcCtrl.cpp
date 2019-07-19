#include <iostream>
#include <sstream>
#include <map>
#include <cassert>
#include <chrono>
#include <thread>
#include <mutex>
#include "pubDef.h"
#include "curlExtThread.hpp"
#include "dcCtrl.h"
#include "appInfo.h"
#include "obj_box_filter.hpp"

using namespace std;

int camState;
curlProcThrd *cq;
map<int,int> cams;
obj_t lastObj;
face_t lastFace;

bool dcInit(int zinLimit, int camId) {
    setZoomInLimit(zinLimit);
    cq = new curlProcThrd("CurlThread", "localhost", 5000);
    cq->run(true);
    char ver[16];

    camState = CAM_ST_OVERVIEW;
    int camRet = cq->getBEVer();
    if (ERR_NONE == camRet) {
        cout << "DC PTZ Ver=" << ver << endl;
    }
    else if (ERR_PENDING == camRet) {
        int runCnt=0;
        while(ERR_PENDING == cq->getBEVerResp(camRet, (void *)ver)){ runCnt++;}
        cout << "DC PTZ Ver=" << ver << " , threadRunTimer=" << runCnt << endl;
    }
    else {
        cout << "errCode=" << camRet << ", errMsg=" << cq->getErrMsg() << endl;
        return false;
    }

    /*
    camRet = cq->getCamList();
    if (ERR_NONE == camRet) {
        cout << "DC PTZ Ver=" << ver << endl;
    }
    else if (ERR_PENDING == camRet) {
        int runCnt=0;
        while(ERR_PENDING == cq->getBEVerResp(camRet, (void *)ver)){ runCnt++;}
        cout << "DC PTZ Ver=" << ver << " , threadRunTimer=" << runCnt << endl;
    }
    else {
        cout << "errCode=" << camRet << ", errMsg=" << cq->getErrMsg() << endl;
        return false;
    }
    */

    return dcZoomOut(camId, cams[camId]);
}

void dcAddCfg(int camId, int presetIdx) {
    cams[camId] = presetIdx;
}

int dcGetState(void) {
    return camState;
}

int reportObj(void *objInfo) {
    int retryLimit = 5;
    int svrRet;
    do {
        svrRet = cq->addObj(objInfo, CAMTYPE_DAHUA);
        cout << "addObj errCode=" << svrRet << ", errMsg=" << cq->getErrMsg() << endl;
    } while ( (ERR_NONE != svrRet)
        && (ERR_PENDING != svrRet)
        && (--retryLimit)
    );
    if (ERR_NONE == svrRet) {
        cout << "addObj done" << endl;
    }
    else {
        cout << "addObj failed" << endl;
        return false;
    }
    return true;
}

int dcUpdateState(int zoomCam) {
    if (CAM_ST_MOVING == camState){
        int cmd, token;
        if (ERR_NONE == cq->checkPendingResp(&cmd, &token)){
            string msg;
            int cmdRst = cq->getResp(cmd, token); 
            if ((ERR_NONE == cmdRst) && (GO_POS == cmd)) {
                camState = CAM_ST_ZOMMIN;
                msg = "CAM_ST_ZOMMIN";
                /* send request to add obj */
                setZoomInTimer(getZoomInLimit());
            }
            else if ((ERR_NONE == cmdRst) && (GO_PRE == cmd)) {
                camState = CAM_ST_OVERVIEW;
                msg = "CAM_ST_OVERVIEW";
            }
            else if (ERR_NONE != cmdRst) {
                /* in case of time-out, always return to preset point*/
                msg = "Restoring from camera error:" + cmdRst;
                camState = CAM_ST_ZOMMIN;
                setZoomInTimer(0);
            }
            cout << msg.c_str() << endl;
        }
    }
    if (CAM_ST_ZOMMIN == camState) {
        if (0==getZoomInTimer()) {
            dcZoomOut(zoomCam, cams[zoomCam]);
        }
        else {
            tickZoomInTimer();
        }
    }
    return camState;
}

bool dcZoomOut(int zoomCam, int presetIdx) {
    int zoomOutLimit = 5;
    int camRet;
    do {
        camRet = cq->goPreset(zoomCam, presetIdx);
        cout << "GoPre errCode=" << camRet << ", errMsg=" << cq->getErrMsg() << endl;
    } while ( (ERR_NONE != camRet)
        && (ERR_PENDING != camRet)
        && (--zoomOutLimit)
    );
    if (ERR_NONE == camRet) {
        cout << "goPre done" << endl;
        camState = CAM_ST_OVERVIEW;
    }
    else {
        if(ERR_PENDING == camRet) {
            camState = CAM_ST_MOVING;
            cout << "CAM_ST_MOVING" << endl;
        }
        else {
            cout << "GoPre failed" << endl;
            return false;
        }
    }
    return true;
}

void fillLastObj(objbox_t *pTarget){
    if (nullptr == pTarget) {
        lastObj.sesnId = -1;
    }
    else {
        /*
        lastObj.classTxt=pTarget->classPtr;
        lastObj.classTxtLen=pTarget->;
        lastObj.sesnId=pTarget->;
        lastObj.left=pTarget->;
        lastObj.top=pTarget->;
        lastObj.width=pTarget->;
        lastObj.height=pTarget->;
        lastObj.conf=pTarget->;
        lastObj.picPath=pTarget->;
        lastObj.picPathLen=pTarget->;
        */
    }
}

bool dcZoomIn(int zoomCam, void *posPtr, void *targetData) {
    dpos_t *pos = (dpos_t *)posPtr;
    int ret = cq->gotoPos(zoomCam, (void *)pos);
    if(ERR_PENDING == ret) {
        camState = CAM_ST_MOVING;
        cout << "CAM_ST_MOVING" << endl;
        // fillLastObj((objbox_t *)targetInfo);
    }
    else {
        cout << "GoPos:errCode=" << ret << "errMsg=" << cq->getErrMsg() << endl;
        return false;
    }
    return true;
}

void dcEnd(void) {
    curlReq_t *req = new curlReq_t;
    req->camID = -1;
    cq->AddToQ(req);
    // curlqt->stop();
    // delete curlqt;
    while(!cq->isDone()){}
    delete cq;
}