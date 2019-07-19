#include <iostream>
#include <sstream>
#include <cassert>

#include "frmFeeder.hpp"

using namespace std;
using namespace cv;

unsigned int ptLimit[PT_NUM] = {1000,60,60,24};

void frmFeederThrd::tickPlaybackTime(unsigned int *pbt, int frmCnt)
{
    if (frmCnt>=0){
        for (int i=0;i<PT_SIZE;i++) {
            pbt[i] = 0;
        }
    }
    
    if(0!=frmCnt) {
        int nFrm = (frmCnt < 0 )?(-frmCnt):frmCnt;
        int nextTick = nFrm * frameDuration;
        for (int i = 0; i < PT_NUM; i++) {
            pbt[i] += nextTick;
            if (pbt[i] >= ptLimit[i]) {
                nextTick = pbt[i] / ptLimit[i];
                pbt[i] -= nextTick * ptLimit[i];
            }
            else {
                break;
            }
        }
    }
    // string info = format("frmFeederThrd::tickPlaybackTime(%d)=%02d:%02d:%02d:%03d", frmCnt, pbt[PT_HOUR], pbt[PT_MIN], pbt[PT_SEC], pbt[PT_MSEC]);
    // cout << info << endl; 
}

void frmFeederThrd::feedLoop(void){
    fRunning = true;
    Mat frame;
    assert(cap.open(src));
    cout << "frmFeederThrd:Open media " << src.c_str() << endl;
    frameCnt=0;
    frame_t *frm = NULL;
    int lastQSize = 1;
    unsigned int feedTime[PT_SIZE];
    resetPlaybackTime(feedTime);
    do {
        if (lastQSize){
            cap >> frame;
            if (frame.empty()) {
                fRunning = false;
                waitKey();
                break;
            }
            /* generate frame structure and dispatch it */
            frm = new frame_t;
            frame.copyTo(frm->frame);
            frm->frameCnt = frameCnt++;
            tickPlaybackTime(feedTime, -1);
            frm->pt[PT_HOUR] = feedTime[PT_HOUR];
            frm->pt[PT_MIN] = feedTime[PT_MIN];
            frm->pt[PT_SEC] = feedTime[PT_SEC];
            frm->pt[PT_MSEC] = feedTime[PT_MSEC];
            frm->flags = 0;
            if (skipCnt >= skipLimit){
                skipCnt = 0;
            }
            else {
                frm->flags |= FF_MASK_SKIP;
                skipCnt++;
            }
            if (fBlocked) {
                frm->flags |= FF_MASK_BLOCK;
            }    
        }
        if (disp) {
            // cout << "frameCnt=" << frm->frameCnt << endl;
            lastQSize = (*disp)(frm, maxBacklog);
        }
    }while(waitKey(1) < 0);
    frm = new frame_t;
    frm->frameCnt = -1;
    if (disp) {
        // cout << "od:skipCnt[st]=" << skipCnt[st] << endl;
        (*disp)(frm, maxBacklog);
    }
    cap.release();
    fRunning = false;
}
