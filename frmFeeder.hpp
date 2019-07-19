#ifndef _FRM_FEEDER_C
#define _FRM_FEEDER_C

#include <cassert>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "pubDef.h"
#include "frmFeeder.h"

using namespace std;
using namespace cv;

typedef int (*frmDisp)(frame_t *, int maxBacklog);

class frmFeederThrd {
private:
    int skipLimit;
    bool fLocalSrc;
    string srcPath;
    string rtspHost;
    int rtspPort;
    string rtspUser;
    string rtspPswd;
    string outputPrefix;
    string outputSuffix;
    frmDisp disp;

    int skipCnt;
    int frameDuration;
    bool fBlocked;
    bool fRunning;
    string src;
    int maxBacklog;
    VideoCapture cap;
    int frameCnt;
    thread _t;

    void tickPlaybackTime(unsigned int *pbt, int frmCnt = -1);
    void resetPlaybackTime(unsigned int *pbt) {
        tickPlaybackTime(pbt, 0);
    }
public:
    frmFeederThrd(frmFeederParams_t *ffp){
        cout<< "frmFeederThrd constructor: create instance " << endl;
        assert(ffp);
        skipLimit = ffp->skipLimit;
        fLocalSrc=ffp->fLocalSrc;
        srcPath=ffp->srcPath;
        rtspHost=ffp->rtsp.host;
        rtspPort=ffp->rtsp.port;
        rtspUser=ffp->rtsp.user;
        rtspPswd=ffp->rtsp.pswd;
        frameDuration = 1000/ffp->fps;
        outputPrefix=ffp->outputPrefix;
        outputSuffix=ffp->outputSuffix;
        disp=(frmDisp)(ffp->func);

        fBlocked = false;
        skipCnt = skipLimit;
        frameCnt = 0;
        if (fLocalSrc) {
            src = srcPath;
        }
        else {
            stringstream uri;
            if (0==rtspPort) {
                uri << "rtsp://" << rtspUser << ":" << rtspPswd << "@" << rtspHost;
            }
            else {
                uri << "rtsp://" << rtspUser << ":" << rtspPswd << "@" << rtspHost << ":" << rtspPort;
            }
            src = string(uri.str());
        }
        if (ffp->qSizeLimit>0) {
            maxBacklog = ffp->qSizeLimit;
        }
        else {
            maxBacklog = 0;
        }
    }
    ~frmFeederThrd(){
        cout<< "frmFeederThrd deconstructor: destroy instance " << endl;
        if(_t.joinable()) {
            _t.join();
        }
    }
    
    void feedLoop(void);

    void setBlock(bool fBlk){
        fBlocked = fBlk;
    }

    bool isRunning(void) {
        return fRunning;
    }

    void run(void) {
        _t = thread(&frmFeederThrd::feedLoop, this);
    }
};
#endif