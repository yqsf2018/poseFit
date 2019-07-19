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
#include "argParser.hpp"
// #include "common.hpp"

#include "obj_box_filter.hpp"
// #include "curlExt.h"
#include "curlExtThread.hpp"
// #include "frameProc.hpp"
#include "bodyDetThread.hpp"
// #include "faceDetThread.hpp"
#include "frmFeeder.hpp"

using namespace std;
using namespace cv;
using namespace dnn;

static const std::string kWinName = "fitPose - Face Tracking";
static const std::string cWinName = "fitPose - Control Panel";
static const std::string fawTBar = "Face Area Width, %";
static const std::string fahTBar = "Face Area Height, %";

bool fAutoZoom = false;
int verboseLevel = 1;
int camPresetIdx = PRESET_DFLT;
int overviewCam = 0;
int zoomCam = 0;


float faceCaptureMargin = FACE_CAP_MRGN_DFLT;
dpos_t faceArea;
std::string target = "All";
vector<string> targets;
std::string modelName;

int compTarget = 0;
int faceFramework = 0;

int frameCnt;
int camState;

// string cqName("curlThread");
curlProcThrd *cq;

mutex stMtx;

frmFeederThrd *feeder;
bodyDetThrd *bt1;
// faceDetThrd *ft1;

extern void endThread(void);

int getCurState(void) {
    return camState;
}

void getScreenResolution(int &width, int &height) {
    Display* disp = XOpenDisplay(NULL);
    Screen*  scrn = DefaultScreenOfDisplay(disp);
    width  = scrn->width;
    height = scrn->height;
    // cout << "wxh=" << width << "," << height << endl;
}

int getVerboseLevel(void) {
    return verboseLevel;
}

void adjVerbLvl(int pos, void*)
{
    verboseLevel = pos;
}

void adjAutoZoom(int pos, void*)
{
    fAutoZoom =  (pos!=0);
}

void adjCamPreset(int pos, void*)
{
    camPresetIdx =  pos;
}

void adjFusionDist(int pos, void*)
{
    setFusionThrsDist(pos);
}

void adjFusionArea(int pos, void*)
{
    setFusionThrsArea(pos);
}

void adjFaceCapMargin(int pos, void*)
{
    faceCaptureMargin = pos * 0.01f;
}

void adjLeaveMax(int pos, void*)
{
    setObjLeaveMax(pos * FPS);
    // cout << "zoomInLimit = " << zoomInLimit << endl;
}

void adjStayMin(int pos, void*)
{
    setObjStayMin(pos * FPS);
    // cout << "zoomInLimit = " << zoomInLimit << endl;
}

void adjZoomInDur(int pos, void*)
{
    zoomInLimit = pos * FPS;
    // cout << "zoomInLimit = " << zoomInLimit << endl;
}

void btAdjBodyThrs ( int pos, void *data) {
    bt1->adjBodyThrs(pos, data); 
}

void btAdjFaceAreaWidth ( int pos, void *data) {
    bt1->adjFaceAreaWidth(pos, data); 
}

void btAdjFaceAreaHeight ( int pos, void *data) {
    bt1->adjFaceAreaHeight(pos, data); 
}

void ftAdjFaceThrs ( int pos, void *data) {
    ft1->adjFaceThrs(pos, data); 
}

void lockRatioCb (int state, void* userdata) {
    cout << "fFaceAreaLockRatio=" << state << endl;
    // fFaceAreaLockRatio = (bool)state;
}

unsigned int getCamState(void) {
    std::unique_lock<std::mutex> lock(stMtx);
    return camState;
}

bool setCamState(unsigned int st) {
    std::unique_lock<std::mutex> lock(stMtx);
    if (st < CAM_ST_NUM) {
        camState = st;
        return true;
    }
    return false;
}

int btAddToQ(frame_t *f) {
    int qSize = bt1->AddToQ(f);
    // cout << "BodyDet:" << qSize << "," << f->frameCnt << "," << (int)(f->flags) << endl;
    if(0==f->frameCnt) {
        cout << "Init NN processing for BodyDet..." << endl;
        while(bt1->isBusy()){
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
        }
    }
}

int main(int argc, char** argv)
{
    bodyDetParams_t BDP;
    assert(parseBodyDetectArg(argc, argv, skipLmt, compTarget, BDP));
    
    int scr_w, scr_h;
    int wndPosX,wndPosY;
    int wndWidth,wndHeight;
    getScreenResolution(scr_w, scr_h);
    wndPosX = scr_w/2;
    wndPosY = WND_POSY_DFLT;
    wndHeight = scr_h/2;
    if (wndHeight < WND_HEIGHT_MIN) {
        wndHeight = WND_HEIGHT_MIN;
    }
    wndWidth = wndHeight * WND_WH_RATIO;
    if (wndWidth < WND_WIDTH_MIN) {
        wndWidth = WND_WIDTH_MIN;
    }

    zoomInLimit = ZOOM_IN_FRM_MAX;
    setNumThreads(CPU_CORE_NUM-1);
    cout << "CV threads=" << getNumThreads() << endl;
    setBreakOnError(true);

    frmFeederParams_t fpp;
    ffp.skipLimit = skipLmt;
    ffp.fLocalSrc = false;
    ffp.srcPath = string("");
    assert (parser.has("input"));
    ffp.srcRtsp = parser.get<String>("input");
    ffp.srcUser = string("admin");
    ffp.srcPswd = string("tnqCAM2in");
    ffp.outputPrefix = string("");
    ffp.outputSuffix = string("");
    ffp.func = ftAddToQ;
    feederBase = new frmFeederThrd(&ffp);

    Thread frmFreeder(feederBase.feedLoop); 

    frmProcParam_t FPP;
    FPP.wName = kWinName;
    FPP.wRect = Rect(wndPosX,wndPosY,wndWidth,wndHeight);
    string tName("BodyDet");
    bt1 = new bodyDetThrd(tName, (void *)&FPP, (void *)&BDP, (void *)cq, zoomCam, BDP.classPath);
    cout << "Load NN:" << BDP.nnModelPath << "," << BDP.nnConfigPath << endl;
    assert(bt1->loadNN(BDP.nnModelPath,BDP.nnConfigPath));
    assert(bt1->setNNCompTarget(compTarget));
    bt1->run();

    cq = new curlProcThrd("CurlThread", "localhost", 5000);
    cq->run();
    char ver[16];
    int camRet = cq->getBEVer();

    if (ERR_NONE == camRet) {
        printf("DC PTZ Ver=%s\n",ver);
    }
    else if (ERR_PENDING == camRet) {
        int runCnt=0;
        while(ERR_PENDING == cq->getBEVerResp(camRet, (void *)ver)){ runCnt++;}
        printf("DC PTZ Ver=%s, threadRunTimer=%d\n",ver, runCnt);
    }
    else {
        printf("errCode=%d, errMsg=%s\n", camRet, cq->getErrMsg());
        return -1;
    }

    wndPosX = WND_POSX_DFLT;
    wndHeight = scr_h/3;
    if (wndHeight < WND_HEIGHT_MIN) {
        wndHeight = WND_HEIGHT_MIN;
    }
    wndWidth = wndHeight * WND_WH_RATIO;
    if (wndWidth < WND_WIDTH_MIN) {
        wndWidth = WND_WIDTH_MIN;
    }
    namedWindow(cWinName, WINDOW_NORMAL);
    moveWindow(cWinName, wndPosX, wndPosY);
    resizeWindow(cWinName, wndWidth, wndHeight);

    int initVerboLvl = verboseLevel;
    createTrackbar("[TBD]Log Level:", cWinName, &initVerboLvl, 5, adjVerbLvl);
    
    int initAutoZoom = fAutoZoom?1:0;
    createTrackbar("[TBD]Auto Zoom:", cWinName, &initAutoZoom, 1, adjAutoZoom);

    int initPreset = camPresetIdx;
    createTrackbar(camPresetTBar, cWinName, &initPreset, 20, adjCamPreset);
    setTrackbarMin(camPresetTBar, cWinName, 1 );

    int initBodyConf = (int)(bt1->getBodyThrs() * 100);
    createTrackbar("Body Conf. Thrs., %", cWinName, &initBodyConf, 99, btAdjBodyThrs);
    
    int initStayMin = getObjStayMin()/FPS;
    createTrackbar(stnThrTBar, cWinName, &initStayMin, 30, adjStayMin);
    setTrackbarMin(stnThrTBar, cWinName, 1 );   

    int initLeaveMax = getObjLeaveMax()/FPS;
    createTrackbar(lvToTBar, cWinName, &initLeaveMax, 30, adjLeaveMax);
    setTrackbarMin(lvToTBar, cWinName, 1 );   

    int initFusionDist = getFusionThrsDist();
    createTrackbar(ftdTBar, cWinName, &initFusionDist, 100, adjFusionDist);
    setTrackbarMin(ftdTBar, cWinName, 5 ); 

    int initFusionArea = getFusionThrsArea();
    createTrackbar(ftasTBar, cWinName, &initFusionArea, 200, adjFusionArea);
    setTrackbarMin(ftasTBar, cWinName, 10);
    
    int initZoomInLim = ZOOM_IN_SEC_MAX;
    createTrackbar("Stare TimeOut, Sec", cWinName, &initZoomInLim, 30, adjZoomInDur);
    
    
    int initFaceConf = (int)(fdp.confThreshold * 100);
    createTrackbar("Face Conf. Thrs., %", cWinName, &initFaceConf, 99, ftAdjFaceThrs);

    int initFAW = bt1->getFaceAreaWidth()*100;
    createTrackbar(fawTBar, cWinName, &initFAW, 80, btAdjFaceAreaWidth);
    
    int initFAH = bt1->getFaceAreaHeight()*100;
    createTrackbar(fahTBar, cWinName, &initFAH, 80, btAdjFaceAreaHeight);

    int initFCM = faceCaptureMargin*100;
    createTrackbar("Capture Margin, %", cWinName, &initFCM, 50, adjFaceCapMargin);
    

    /* Open a video file or an image file or a camera stream. */
    string capSrc;
    

    cout << "Reset Cam Pos" << endl;
    camRet = cq->goPreset(zoomCam, camPresetIdx);
    if (ERR_PENDING == camRet) {
        setCamState(CAM_ST_MOVING);
        cout << "CAM_ST_MOVING" << endl;
    }

    int skipCnt[CAM_ST_NUM] = {
        0,0,0
    };
    int skipLimit[CAM_ST_NUM] = {
        1,1,1
    };
    for (int i=0;i<CAM_ST_NUM;i++) {
        skipLimit[i]=skipLmt;
        skipCnt[i]=skipLimit[i];
    }
    VideoCapture cap;
    Mat frame;
    assert(cap.open(capSrc));

    int frameCnt=0;
    int st=-1, prevSt;
    int (*frmHandler[])(frame_t *) = {
        btAddToQ,
        nullptr,
        ftAddToQ,
    };

    frame_t *frm = new frame_t;
    cap >> (frm->frame);
    frm->frameCnt = 0;
    frm->flags = 0;
    btAddToQ(frm);

    frm = new frame_t;
    cap >> (frm->frame);
    frm->frameCnt = 0;
    frm->flags = 0;
    ftAddToQ(frm);

    cout << "FF_MASK_SKIP=" << FF_MASK_SKIP << endl;
    /* main detection loop */
    do {
        cap >> frame;
        if (frame.empty()) {
            waitKey();
            break;
        }
        prevSt = st;
        st = getCamState();
        if(st!=prevSt) {
            skipCnt[st] = skipLimit[st];
        }
        /* generate frame structure and dispatch it */
        frm = new frame_t;
        frm->frame = frame.clone();
        frm->frameCnt = ++frameCnt;
        frm->flags = 0;
        if (skipCnt[st] >= skipLimit[st]){
            skipCnt[st] = 0;
        }
        else {
            frm->flags |= FF_MASK_SKIP;
            skipCnt[st]++;
        }

        if (frmHandler[st]) {
            // cout << "od:skipCnt[st]=" << skipCnt[st] << endl;
            frmHandler[st](frm);
        }
        
        /* cam FSM */
        if (CAM_ST_MOVING == st){
            int cmd, token;
            if (ERR_NONE == cq->checkPendingResp(&cmd, &token)){
                int cmdRst = cq->getResp(cmd, token); 
                if ((ERR_NONE == cmdRst) && (GO_POS == cmd)) {
                    setCamState(CAM_ST_ZOMMIN);
                    cout << "CAM_ST_ZOMMIN" << endl;
                    zoomInTimer = zoomInLimit;
                }
                else if ((ERR_NONE == cmdRst) && (GO_PRE == cmd)) {
                    setCamState(CAM_ST_OVERVIEW);
                    cout << "CAM_ST_OVERVIEW" << endl;
                }
                else if (ERR_NONE != cmdRst) {
                    /* in case of time-out, always return to preset point*/
                    cout<<"Restoring from camera error:" << cmdRst << endl;
                    setCamState(CAM_ST_ZOMMIN);
                    zoomInTimer = 0;
                }
            }
        }
        if (CAM_ST_ZOMMIN == st) {
            if (0==zoomInTimer) {
                int zoomOutLimit = 5;
                do {
                    camRet = cq->goPreset(zoomCam, camPresetIdx);
                    printf("GoPre errCode=%d, errMsg=%s\n", camRet, cq->getErrMsg());
                } while ( (ERR_NONE != camRet)
                    && (ERR_PENDING != camRet)
                    && (--zoomOutLimit)
                );
                if (ERR_NONE == camRet) {
                    printf("goPre done\n");
                    setCamState(CAM_ST_OVERVIEW);
                }
                else {
                    if(ERR_PENDING == camRet) {
                        setCamState(CAM_ST_MOVING);
                        cout << "CAM_ST_MOVING" << endl;
                        string zLabel = "Zoom Out";
                        int baseLine;
                        Size labelSize = getTextSize(zLabel, FONT_HERSHEY_SIMPLEX, 4, 3, &baseLine);
                        putText(frame, zLabel, Point(frame.cols/2-labelSize.width/2,frame.rows/2-labelSize.height/2)
                        , FONT_HERSHEY_SIMPLEX, 5, BGR_RED, 2);
                    }
                    else
                    {
                        cout << "GoPre failed" << endl;
                        return -3;
                    }  
                } 
            }
            else {
                zoomInTimer--;
            }
        }
/*      else if (CAM_ST_OVERVIEW == st) {
            if (++skipCnt >= skipLimit){
                skipCnt = 0; 
                // Create a 4D blob from a frame.
                Size inpSize(inpWidth > 0 ? inpWidth : frame.cols,
                             inpHeight > 0 ? inpHeight : frame.rows);
                blobFromImage(frame, blob, scale, inpSize, b_mean, swapRB, false);

                // Run a model.
                net.setInput(blob);
                if (net.getLayer(0)->outputNameToIndex("im_info") != -1) { // Faster-RCNN or R-FCN 
                    resize(frame, frame, inpSize);
                    Mat imInfo = (Mat_<float>(1, 3) << inpSize.height, inpSize.width, 1.6f);
                    net.setInput(imInfo, "im_info");
                }
                std::vector<Mat> cur_outs;
                net.forward(cur_outs, outNames);
                outs = cur_outs;
                std::vector<double> layersTimes;
                double freq = getTickFrequency();
                double tt = net.getPerfProfile(layersTimes) / freq;
                label = format("%.3f mS, %.2f FPS", 1000*tt, 1/tt);
            }
            else {
                skipCnt++;
            }
            postprocess(frame, outs, net);
            // Put efficiency information.
            wholeAreaInfo = format("Fusion(%d,%d)-D:%d,P:%d,C:%d-%s", getFusionThrsDist2(), getFusionThrsArea2(), ovInfo.nDetected, ovInfo.nPromise, ovInfo.nCandiate, label.c_str());
            putText(frame, wholeAreaInfo.c_str(), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, BGR_BLUE, 2);
            rectangle(frame, Point(frame.cols/2 - ovInfo.target_w/2, frame.rows/2 - ovInfo.target_h/2)
                    , Point(frame.cols/2 + ovInfo.target_w/2, frame.rows/2 + ovInfo.target_h/2), BGR_YELLOW, 2);
            imshow(kWinName, frame);
        } */
        if (bt1->isAdjOvTargetUpdated()) {
            bt1->updateOvTarget(frame);
        }
    } while(waitKey(1) < 0);

    /* stop processing threads */
    frame_t *e1 = new frame_t;
    e1->frameCnt = -1;
    bt1->AddToQ(e1);

    frame_t *e2 = new frame_t;
    e2->frameCnt = -1;
    ft1->AddToQ(e2);

    while(!ft1->isDone()){
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
    }
    delete ft1;

    while(!bt1->isDone()){
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
    }
    delete bt1;
    
    /* stop Curl process thread */
    curlReq_t *req = new curlReq_t;
    req->camID = -1;
    cq->AddToQ(req);
    // curlqt->stop();
    // delete curlqt;
    while(!cq->isDone()){}
    delete cq;
    
    /* stop picture processing thread */
    endThread(); 
    
    return 0;
}