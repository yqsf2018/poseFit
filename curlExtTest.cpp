#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include "curlExt.h"
#include "curlExtThread.hpp"

using namespace std;

curlProcThrd *curlqt;

void test_readPTZ(int camId, ptz_t *dhPtz) {
    int token;
    int ret = curlqt->readPTZ(camId, &token);
    if(ERR_PENDING != ret) {
        cout << "ret = " << ret << endl;
        assert(0);
    }
    do {
        ret = curlqt->getPTZResp(ret, token, (void *)dhPtz);
    }while(ret==ERR_PENDING);

    if (ERR_NONE == ret) {
        printf("pan=%d, tile=%d, zoom=%d, zoomValue=%d\n",dhPtz->pan,dhPtz->tilt,dhPtz->zoom,dhPtz->zoomVal);
    }
    else {
        printf("errCode=%d, errMsg=%s\n", ret, curlqt->getErrMsg());
    }
}

int test_setPTZ(int camId, ptz_t *dstPtz) {
    int token, cmd;

    int ret = curlqt->movePTZ(camId, (void *)dstPtz);
    // int ret = setPTZ(camId, "webhook.site", 443, (void *)dstPtz);
    while(ERR_PENDING == curlqt->checkPendingResp(&cmd, &token)){
        
    }
    ret = curlqt->getResp(cmd, token);
    if (ERR_NONE == ret) {
        printf("setPTZ done\n");
    }
    else {
        printf("errCode=%d, errMsg=%s\n", ret, curlqt->getErrMsg());
    }
    return ret;
}

void test_goPre(int camId, int refId) {
    int token, cmd;

    int ret = curlqt->goPreset(camId, refId);
    while(ERR_PENDING == curlqt->checkPendingResp(&cmd, &token)){}
    ret = curlqt->getResp(cmd, token);
    if (ERR_NONE == ret) {
        printf("goPre done\n");
    }
    else {
        printf("errCode=%d, errMsg=%s\n", ret, curlqt->getErrMsg());
    }
}

void test_savePre(int camId, int refId) {
    int token, cmd;

    int ret = curlqt->savePreset(camId, refId);
    while(ERR_PENDING == curlqt->checkPendingResp(&cmd, &token)){}
    ret = curlqt->getResp(cmd, token);
    if (ERR_NONE == ret) {
        printf("savePre done\n");
    }
    else {
        printf("errCode=%d, errMsg=%s\n", ret, curlqt->getErrMsg());
    }
}

int test_goPos(int camId, dpos_t *pos) {
    int token, cmd;

    int ret = curlqt->gotoPos(camId, (void *)pos);
    while(ERR_PENDING == curlqt->checkPendingResp(&cmd, &token)){}
    ret = curlqt->getResp(cmd, token);
    if (ERR_NONE == ret) {
        printf("goPos done\n");
    }
    else {
        printf("errCode=%d, errMsg=%s\n", ret,curlqt->getErrMsg());
    }
    return ret;
}

int main(int argc, char* argv[]) {
    ptz_t dhPtz;
    ptz_t dstPtz;
    int cam_id, ref_id;

    char ver[16];
    if (argc >1) {
        cam_id = atoi(argv[1]);
        cout << "cam_id=" << cam_id << endl;
    }
    if (argc>2){
        ref_id = atoi(argv[2]);
        cout << "ref_id=" << ref_id << endl;
    }

    curlqt = new curlProcThrd("CurlThreadTest", "localhost", 5000);
    curlqt->run(true);

    int ret = curlqt->getBEVer();
    do {
        ret = curlqt->getBEVerResp(ret, (void *)ver);
    }while(ret==ERR_PENDING);
    if (ERR_NONE == ret) {
        printf("ver=%s\n",ver);
    }
    else {
        printf("errCode=%d, errMsg=%s\n", ret, curlqt->getErrMsg());
    }

    //test_readPTZ(cam_id, &dhPtz);
    
    dstPtz.pan = 100;
    dstPtz.tilt = 30;
    dstPtz.zoom = 20;
    ret = test_setPTZ(cam_id, &dstPtz);
    if (ERR_NONE == ret){
        do {
            test_readPTZ(cam_id, &dhPtz);
        }while((dstPtz.zoom - dhPtz.zoom)*(dstPtz.zoom - dhPtz.zoom) > 4);
    }
    
    test_goPre(cam_id, ref_id);
    do {
        test_readPTZ(cam_id, &dhPtz);
    }while(dhPtz.zoom>3);

    test_savePre(cam_id, ref_id);

    dpos_t pos;
    pos.x= 1600;
    pos.y= 1200;
    pos.dzoom = 30;
    ret = test_goPos(cam_id, &pos);
    if (ERR_NONE == ret){
        test_readPTZ(cam_id, &dhPtz);
    }
    
    curlReq_t *req = new curlReq_t;
    req->camID = -1;
    curlqt->AddToQ(req);
    // curlqt->stop();
    // delete curlqt;
    while(!curlqt->isDone()){}
    delete curlqt;
}