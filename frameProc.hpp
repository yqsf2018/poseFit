#ifndef _FRM_PROC
#define _FRM_PROC

#include <cassert>
#include <thread>
#include <iostream>
#include <string>
#include <sstream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>
#include "pubDef.h"
#include "picQ.hpp"

#include "fifoProc.hpp"
using namespace std;
using namespace cv;
using namespace cv::dnn;

typedef struct {
    string wName;
    Rect wRect;
    string cName;
    Rect cRect;  
} frmProcParam_t;

class frmProcThrd : public fifoProcThrd<frame_t> {
protected:
    string wndName;
    string cpName;
    Rect wndRect;
    Rect cpRect;
    bool fShowFrm;
    bool fCPanel;
    bool fNetReady;
    VideoWriter *scr; 
// protected:
    // Net net;
// protected:
    // vector<String>  outNames;
public:
    frmProcThrd(string &name, void *frmProcParamPtr) : fifoProcThrd<frame_t>(name) {
        cout<< "frmProcThrd constructor: create instance " << this->getThrdName() << endl;
        assert(frmProcParamPtr);
        frmProcParam_t *fpp = (frmProcParam_t *)frmProcParamPtr;
        wndName = fpp->wName;
        wndRect = std::move(fpp->wRect);
        cpName = fpp->cName;
        cpRect = std::move(fpp->cRect);
        
        fShowFrm = !(wndName.empty() && wndRect.empty());
        fCPanel = !(cpName.empty() && cpRect.empty());
        fNetReady = false;
        if (fShowFrm) {
            cout<< "Show Frame:" << wndName << endl;
            namedWindow(wndName, WINDOW_NORMAL);
            moveWindow(wndName, wndRect.x, wndRect.y);
            resizeWindow(wndName, wndRect.width, wndRect.height);
        }
        if (fCPanel) {
            cout<< "Show CPanel:" << cpName << endl;
            namedWindow(cpName, WINDOW_NORMAL);
            moveWindow(cpName, cpRect.x, cpRect.y);
            resizeWindow(cpName, cpRect.width, cpRect.height);
        }
        scr = new VideoWriter("bDetTestOut.avi",VideoWriter::fourcc('M','J','P','G'),25, Size(cpRect.width,cpRect.width));
    }
    ~frmProcThrd(){
        cout<< "frmProcThrd deconstructor: destroy instance " << this->getThrdName() << endl;
        scr->release();
        delete scr;
    }    

    virtual void preThreadStart (void){
        cout << "frmProcThrd::preThreadStart():" << getThrdName() << endl;
    }

    virtual void postThreadLoop (void){
        cout << "frmProcThrd::postThreadLoop():" << getThrdName() << endl;
    }

    virtual void threadStartNotify (void){
        stringstream info;
        info << "FrameProc Thread " << this->getThrdName() << " started" << endl;
        cout<< info.str() << endl;
    }

    virtual void threadEndNotify (void){
        stringstream info;
        info << "FrameProc Thread " << this->getThrdName() << " ended" << endl;
        cout<< info.str() << endl;
    }

    virtual bool loadNet (string &modelPath, string &configPath, Net &net) {
        /* switch(fdp.framework) {
        case 1:
          modelName = "TF";
          f_net = cv::dnn::readNetFromTensorflow(fdp.modelPath, fdp.configPath);
          break;
        case 0:
        default:
          modelName = "CAFFE";
          f_net = cv::dnn::readNetFromCaffe(fdp.configPath, fdp.modelPath);
        } */
        // fLoaded = false;
        if (modelPath.empty() || configPath.empty()){
            cout << "Invalide NN file path" << endl;
        }
        else {
            double tStart = cv::getTickCount();
            net = readNet(modelPath, configPath);
            // net.setPreferableBackend(parser.get<int>("backend"));
            net.setPreferableBackend(DNN_BACKEND_INFERENCE_ENGINE);
            double tLoad = 1000*((double)cv::getTickCount() - tStart)/cv::getTickFrequency();
            cout << format("Load DNN for %s...%.3f mS", this->getThrdName(), tLoad) << endl;
            // outNames = getOutputsNames(net);
            // fLoaded = true;
            fNetReady = true;
        }
        return fNetReady;
    }

    bool setCompTarget(int compTarget, Net &net) {
        if (fNetReady) {
            net.setPreferableTarget(compTarget);
            return true;
        }
        return false;
    }

    virtual void processFrame (frame_t *f) { }

    virtual void processItem(frame_t *f){
        if (-1 == f->frameCnt) {
            cout << "stop in " << this->getThrdName() << " in frmProcThrd::processItem()" << endl;
            this->stop();
            return;
        }
        //cout << "frmProcThrd:processItem:processFrame:" << f->frameCnt << endl;
        processFrame(f);
        //cout << "frmHandler:" << f->frameCnt << "," << fShowFrm << "," << wndName << endl;
        if (!isDone()) {
            if(fShowFrm) {
                // cout << "Thread-" <<  getThrdName() << ": show frame #" << f->frameCnt << endl;
                imshow(wndName, f->frame);
            }
            if ( CHECK_FLAG(f->flags, FF_MASK_RECORD) ) {
                scr->write(f->frame);
            }
        }
    }
    /*virtual void stop(){
        cout << "frmProcThrd::stop()" << endl;
    }*/
};
#endif