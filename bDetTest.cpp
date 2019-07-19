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
#include "dcCtrl.h"
#include "obj_box_filter.hpp"
#include "bodyDetThread.hpp"
#include "frmFeeder.h"
#include "frmFeeder.hpp"
// #include "frameProc.hpp"

using namespace std;
using namespace cv;
using namespace dnn;

static const std::string kWinName = "fitPose - Body Detetion";
static const std::string cWinName = "fitPose - Control Panel I";

bodyDetThrd *bt1;

enum barIndex {
    CONF_THRS = 0
    ,FUD_THRS
    ,FUAS_THRS
    ,STA_THRS
    ,LEAVE_TO
    ,STARE_TO
    ,CAM_PRESET
    ,FAW_PERC
    ,FAH_PERC
    ,PANEL_VAR_NUM
};

static const std::string barName[PANEL_VAR_NUM] = {
    "Body Conf. Thrs., %"
    ,"Fusion Thrs. Dist."
    ,"Fusion Thrs. Area Side"
    ,"Stationary Thrs., Sec"
    ,"Leave Timeout, Sec"
    ,"Stare TimeOut, Sec"
    ,"[TBD]CAM PRESET:"
    ,"Face Area Width, %"
    ,"Face Area Height, %"
};

const int barVarMin[PANEL_VAR_NUM] = {
    -1,5,10,1,1,-1,1,-1,-1
};

const int barVarMax[PANEL_VAR_NUM] = {
    99,100,200,30,30,30,20,80,80 
};

int barVarDflt[PANEL_VAR_NUM] = {
    80
    ,OBJ_MERGE_DIST_MAX
    ,OBJ_MERGE_AREA_DIFF_MAX
    ,OBJ_STAY_MIN
    ,OBJ_LEAVE_MAX
    ,ZOOM_IN_SEC_MAX
    ,10
    ,50
    ,50
};

int qFullStat = 0;
int lastFrameCnt = 0;
int btAddToQ(frame_t *f, int maxBackLog) {
    int qSize=bt1->getQSize();
    if ((maxBackLog>0) && (qSize>=maxBackLog) ) {
        if (f->frameCnt != lastFrameCnt) {
            qFullStat++;
            lastFrameCnt = f->frameCnt;
        }
        // cout << "input Q >=" << maxBackLog << "...hold, stat=" << qFullStat << endl;
        return 0;
    }
    qSize = bt1->AddToQ(f);
    //cout << "BodyDet:" << qSize << "," << f->frameCnt << endl;
    if( (f->frameCnt<=10) && (f->frameCnt>0) ) {
        cout << "Init NN processing for BodyDet..." << f->frameCnt << endl;
        while(bt1->isBusy()){
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
        }
        cout << "done" << endl;
    }
    return qSize;
}

void getScreenResolution(int &width, int &height) {
    Display* disp = XOpenDisplay(NULL);
    Screen*  scrn = DefaultScreenOfDisplay(disp);
    width  = scrn->width;
    height = scrn->height;
    // cout << "wxh=" << width << "," << height << endl;
}

int  barVar[PANEL_VAR_NUM];

void adjCamPreset(int pos, void*)
{
    bt1->adjCamPreset(pos);
    cout << "camPreset = " << bt1->getCamPreset() << endl;
}

void adjFusionDist(int pos, void*)
{
    setFusionThrsDist(pos);
    cout << "Fusion Dist. Threshold = " << getFusionThrsDist() << "," << getFusionThrsDist2() << endl;
}

void adjFusionArea(int pos, void*)
{
    setFusionThrsArea(pos);
    cout << "Fusion Area Threshold = " << getFusionThrsArea() << "," << getFusionThrsArea2() << endl;
}

void adjLeaveMax(int pos, void*)
{
    setObjLeaveMax(pos * FPS);
    cout << "ObjLeaveMax = " << getObjLeaveMax() << endl;
}

void adjStayMin(int pos, void*)
{
    setObjStayMin(pos * FPS);
    cout << "ObjStayMin = " << getObjStayMin() << endl;
}

void adjZoomInDur(int pos, void*)
{
    setZoomInLimit(pos * FPS);
    cout << "zoomInLimit = " << getZoomInLimit() << endl;
}

void btAdjBodyThrs ( int pos, void *data) {
    bt1->adjBodyThrs(pos);
    cout << "Body Det Threshold = " << bt1->getBodyThrs() << endl;
}

void btAdjFaceAreaWidth ( int pos, void *data) {
    bt1->adjFaceAreaWidth(pos);
    cout << "Face Area Width Perc = " << bt1->getFaceAreaWidth() << endl;
}

void btAdjFaceAreaHeight ( int pos, void *data) {
    bt1->adjFaceAreaHeight(pos);
    cout << "Face Area Height Perc = " << bt1->getFaceAreaHeight() << endl;
}

void (* adjPanelVar[PANEL_VAR_NUM])(int pos, void*) = {
    btAdjBodyThrs
    ,adjFusionDist
    ,adjFusionArea
    ,adjStayMin
    ,adjLeaveMax
    ,adjZoomInDur
    ,adjCamPreset
    ,btAdjFaceAreaWidth
    ,btAdjFaceAreaHeight
};

bool initCPanel (string &cWinName) {
    barVar[CONF_THRS] = bt1->getBodyThrs() * 100;
    barVar[FUD_THRS] = getFusionThrsDist();
    barVar[FUAS_THRS] = getFusionThrsArea();
    barVar[STA_THRS] = getObjStayMin();
    barVar[LEAVE_TO] = getObjLeaveMax();
    barVar[STARE_TO] = getObjStayMin();
    barVar[CAM_PRESET] = bt1->getCamPreset();
    barVar[FAW_PERC] = 100*bt1->getFaceAreaWidth();
    barVar[FAH_PERC] = 100*bt1->getFaceAreaHeight();

    for (int i=0;i<PANEL_VAR_NUM;i++) {
        createTrackbar(barName[i], cWinName, barVar+i, barVarMax[i], adjPanelVar[i]);
        if (-1 != barVarMin[i]) {
            setTrackbarMin(barName[i], cWinName, barVarMin[i] );
        }
    }
}

void initScreen(frmProcParam_t &FPP) {
    int scr_w, scr_h;
    int wndPosX,wndPosY;
    int wndWidth,wndHeight;
    getScreenResolution(scr_w, scr_h);
    wndPosX = scr_w/2 + WND_POSX_DFLT;
    wndPosY = 2*WND_POSY_DFLT;
    wndHeight = scr_h/2;
    if (wndHeight < WND_HEIGHT_MIN) {
        wndHeight = WND_HEIGHT_MIN;
    }
    wndWidth = wndHeight * WND_WH_RATIO;
    if (wndWidth < WND_WIDTH_MIN) {
        wndWidth = WND_WIDTH_MIN;
    }
    
    FPP.wName = kWinName;
    FPP.cName = cWinName;
    FPP.wRect = Rect(wndPosX,wndPosY,wndWidth,wndHeight);
    FPP.cRect = Rect(WND_POSX_DFLT,wndPosY,wndWidth,wndHeight);
}

int main(int argc, char** argv)
{
    frmFeederParams_t ffp;
    /* model parameters for body detection */
    bodyDetParams_t bdp;
    if(!parseBodyDetectArg(argc, argv, (void*)&ffp, bdp)) {
        exit(-1);
    }

    /* initialize displays */
    frmProcParam_t FPP;
    initScreen(FPP);

    // dcAddCfg(0, 10);
    // dcAddCfg(1, 10);
    // dcInit(ZOOM_IN_FRM_MAX, 0);

    /* body detection main body */
    string tName = "BodyDetTest";
    bt1 = new bodyDetThrd(tName, (void *)&FPP, (void *)&bdp, 1);
    cout << "Load NN:" << bdp.modelPath << "," << bdp.configPath << endl;
    assert(bt1->loadNN(bdp.modelPath, bdp.configPath));
    assert(bt1->setNNCompTarget(bdp.compTarget));
 
    /* initialize frame feeding thread */
    ffp.func = (void *)btAddToQ;
    frmFeederThrd *feeder = new frmFeederThrd(&ffp);
    obj_box_init();
    initCPanel(FPP.cName);

    // setZoomInTimer(1);
    /* start feeding */
    feeder->run();
    /* start handling */
    bt1->run(false);

    /* terminate both feeding and handling threads*/
    delete feeder;
    delete bt1;
    // dcEnd();
    return 0;
}