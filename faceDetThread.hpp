#ifndef _FDET_THREAD
#define _FDET_THREAD

#include <cassert>
#include <string>
#include <fstream>
#include <thread>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include "pubDef.h"
#include "argParser.h"
#include "frameProc.hpp"
// #include "faceDetThread.h"
#include "postProcPic.hpp"
#include "bodyDetThread.hpp"

using namespace std;
using namespace cv;
using namespace dnn;

class faceDetThrd : public frmProcThrd {
public:
    float scale;
    int inpWidth;
    int inpHeight;
    float confThreshold;
    Scalar mean;
    bool swapRB;
    int framework;
    int wLim;
    int hLim;
    float faceCaptureMargin;
    string modelName;
    bool fFaceRect;

    // bool fShowFrm;
    // bool fNetReady;
    Net net;

    mediaQueue<pic_t> mq;

    std::vector<Mat> outs;
    
    /* frame processing parameters */
    stat_t faceCnt;

    bool validFace(int w,int h) {
        return ( (w>=wLim) && (h>=hLim) );
    }

    void newSesn(stat_t &c) {
        faceCnt.sesn++;
        faceCnt.idx = 0;
    }

    void incSesn(stat_t &c) {
        c.idx++;
        c.tot++;
    }

    void resetStat(stat_t &c) {
        c.sesn = 1;
        c.idx = 0;
        c.tot = 0;
    }
    int detectFaceOpenCVDNN(Mat &frameOpenCVDNN);
    void initPicThread(void);
    void endPicThread(void);

public:
    faceDetThrd(string &name, void *frmProcParamPtr, void *faceDetParamPtr) : frmProcThrd(name, frmProcParamPtr) {
        cout<< "faceDetThrd constructor: create instance " << this->getThrdName() << endl;
        assert(faceDetParamPtr);
        faceDetParams_t *fdp = (faceDetParams_t *)faceDetParamPtr;
        inpWidth = fdp->inpWidth;
        inpHeight = fdp->inpHeight;
        confThreshold = fdp->confThreshold;
        scale = fdp->scale; 
        mean = fdp->mean;
        swapRB = fdp->swapRB;
        framework = fdp->framework;
        wLim = fdp->wLim;
        hLim = fdp->hLim;
        faceCaptureMargin = fdp->faceCaptureMargin;
        fFaceRect = true;
        resetStat(faceCnt);
        if (fdp->framework) {
            modelName = string("TF");
        }else {
            modelName = string("Caffe");
        }
        initPicThread();
    }
    ~faceDetThrd(){
        endPicThread();
        cout<< "faceDetThrd deconstructor: destroy instance " << this->getThrdName() << endl;
    }
    
    bool loadNN(string &modelPath, string &configPath) {
        return loadNet(modelPath, configPath, net);
    }

    bool setNNCompTarget(int compTarget){
        return setCompTarget(compTarget, net);
    }

    void adjFaceThrs(int pos, void*) {
        confThreshold = pos * 0.01f;
    }

    virtual void processFrame (frame_t *f);
};

#endif