#ifndef _BDET_THREAD
#define _BDET_THREAD

#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include "pubDef.h"
#include "argParser.h"
#include "frameProc.hpp"
#include "curlExtThread.hpp"
#include "bodyDetThread.h"

using namespace std;
using namespace cv;
using namespace cv::dnn;

typedef struct {
    int typeName;
    double scale;
    cv::Scalar color;
    int thickness;
} fontAttr_t;

typedef struct {
    int bottom;
    int left;
    fontAttr_t attr;
    std::string txtLine;
} frameCaption_t;

class bodyDetThrd : public frmProcThrd {
private:
    bool fAdjOvTarget;
    // string wndName;
    // Rect wndRect;
    // bool fShowFrm;
    // bool fNetReady;
    Net net;
    Mat blob;
    std::vector<Mat> outs;
    std::vector<std::string> classes;
    overView_t ovInfo;
    faceView_t fvInfo;
    
    string target;
    float faceAreaPerc_w, faceAreaPerc_h;
    float faRatio;
    bool fFaceAreaLockRatio;
    bool fShowFaceTargetRect;
    string labelTxt;
    int capHeadX;
    int capHeadY;
    int capLineMarginX;
    int capLineMarginY;
    std::vector<frameCaption_t> capList;

    /* frame processing parameters */
    int inpWidth, inpHeight;
    float scale;
    Scalar b_mean;
    bool swapRB;
    float bodyConfThreshold;
    float nmsThreshold;

    int zoomCam;
    int camPresetIdx;
    int camNum;
    int fps;

    void addCaption(string &capTxt, int top=0,int left=0
    , int fontType=FONT_HERSHEY_SIMPLEX
    , double fontSize = 1.0
    , cv::Scalar clr = BGR_BLUE
    , int thick = 2);
    string getObjLabel(float conf,int id, Rect *rect);
    void drawPred(string label, int labelPos, Rect &rect, Mat& frame, Scalar &lineColor, int lineThickness);
    void postprocess(Mat& frame, int frameCnt);
    int calcZoomRatio(int view_w, int view_h, int area_w, int area_h, float *zoomRatio);
    void updateFaRatio(void) {
        faRatio = faceAreaPerc_h/faceAreaPerc_w;
    }
public:
    bodyDetThrd(string &name, void *frmProcParamPtr, void *bodyDetParamPtr, int camId) : frmProcThrd(name, frmProcParamPtr) {
        cout<< "bodyDetThrd constructor: create instance " << this->getThrdName() << endl;
        assert(bodyDetParamPtr);
        fAdjOvTarget=true;
        fFaceAreaLockRatio = false;
        fShowFaceTargetRect = false;
        faceAreaPerc_w = FAW_DFLT;
        faceAreaPerc_h = FAH_DFLT;
        bodyDetParams_t *bdp = (bodyDetParams_t *)bodyDetParamPtr;
        inpWidth = bdp->inpWidth;
        inpHeight = bdp->inpHeight;
        scale = bdp->scale;
        b_mean = bdp->b_mean;
        swapRB = bdp->swapRB;
        target = bdp->target;
        bodyConfThreshold = bdp->bodyConfThreshold;
        nmsThreshold = bdp->nmsThreshold;
        zoomCam = camId;

        capList.clear();
        capLineMarginX = 5;
        capLineMarginY = 5;
        capHeadX = capLineMarginX;
        capHeadY = capLineMarginY;

        camPresetIdx = CAM_PRESET_DFLT;
        /* load class file */
        std::ifstream ifs(bdp->classPath);
        if (!ifs.is_open()){
            CV_Error(Error::StsError, "File " + bdp->classPath + " not found");
        }
        std::string line;
        while (std::getline(ifs, line)){
            classes.push_back(line);
        }
        cout << "Loaded class file " << bdp->classPath << endl;
    }
    ~bodyDetThrd(){
        cout<< "bodyDetThrd deconstructor: destroy instance " << this->getThrdName() << endl;
    }

    void resetCapLoc(void) {
        capHeadX = capLineMarginX;
        capHeadY = capLineMarginY;
    }

    bool validTarget(int classId) {
        return ("All"==target) || (classes[classId] == target) ; 
    }

    bool loadNN(string &modelPath, string &configPath) {
        return loadNet(modelPath, configPath, net);
    }
    
    bool setNNCompTarget(int compTarget){
        return setCompTarget(compTarget, net);
    }

    vector<String> getOutputsNames(void);
    virtual void processFrame (frame_t *f);

    int getCamPreset(void) { return camPresetIdx; }
    void adjCamPreset(int pos) { camPresetIdx = pos;}

    float getBodyThrs(void) { return bodyConfThreshold; }
    void adjBodyThrs(int pos) { bodyConfThreshold = pos * 0.01f;}
    
    float getFaceAreaWidth(void) { return faceAreaPerc_w; }
    void adjFaceAreaWidth(int pos);
    
    float getFaceAreaHeight(void) { return faceAreaPerc_h; }
    void adjFaceAreaHeight(int pos);
    
    bool isAdjOvTargetUpdated(void) { return fAdjOvTarget; }
    void setAdjOvTargetUpdated(bool fVal) { fAdjOvTarget = fVal; }
    
    void updateOvTarget(Mat& frame) {
        ovInfo.target_w = (int)(frame.cols*faceAreaPerc_w+0.5);
        ovInfo.target_h = (int)(frame.rows*faceAreaPerc_h+0.5);
        fAdjOvTarget=false;
    }

    int getCandidateIdx(void) { return fvInfo.candidateIdx; }
    bool initCPanel(void);
};
#endif