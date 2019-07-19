#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <X11/Xlib.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>

#include "pubDef.h"
#include "argParser.h"
#include "appInfo.h"

#include "picQ.hpp"
#include "postProcPic.hpp"
#include "frmFeeder.h"
#include "frmFeeder.hpp"
#include "frameProc.hpp"
#include "faceCapture.h"

using namespace std;
using namespace cv;
using namespace cv::dnn;

static const std::string kWinName = "fitPose - Face Detetion";
static const std::string cWinName = "fitPose - Control Panel II";

//#define CAFFE

string appName("FaceDet");

int compTarget;

mediaQueue<frame_t> *fifoQ;
bool fProcessing = false;
bool fRunLoop = false;
bool fShowFrm = true;
stat_t faceCnt;
mediaQueue<pic_t> mq;

cv::dnn::Net net;
vector<Mat> outs;
bool fNetReady = false;

float scale;
int inpWidth;
int inpHeight;
float confThreshold;
Scalar blobMean;
bool swapRB;
int framework;
int wLim;
int hLim;
float faceCaptureMargin;
string modelName;
string picDepotPath, picPrefix, picSuffix;

time_t rawtime;
struct tm * ptm = NULL;

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

bool validFace(int w,int h) {
    return ( (w>=wLim) && (h>=hLim) );
}

int detectFaceOpenCVDNN(Mat &frameOCD)
{
    bool fDetected = false;

    int frameHeight = frameOCD.rows;
    int frameWidth = frameOCD.cols;
    Size inpSize(inpWidth > 0 ? inpWidth : frameOCD.cols,
                inpHeight > 0 ? inpHeight : frameOCD.rows);
    

    cv::Mat inputBlob = cv::dnn::blobFromImage(frameOCD, scale, inpSize, blobMean, swapRB, false);
    // cout << "inputBlob=" << inputBlob << endl;
    net.setInput(inputBlob, "data");
    cv::Mat detection = net.forward("detection_out");

    cv::Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());
    for(int i = 0; i < detectionMat.rows; i++) {
        float confidence = detectionMat.at<float>(i, 2);

        if(confidence > confThreshold) {
            int x1 = static_cast<int>(detectionMat.at<float>(i, 3) * frameWidth);
            int y1 = static_cast<int>(detectionMat.at<float>(i, 4) * frameHeight);
            int x2 = static_cast<int>(detectionMat.at<float>(i, 5) * frameWidth);
            int y2 = static_cast<int>(detectionMat.at<float>(i, 6) * frameHeight);
            int w = x2-x1+1;
            int h = y2-y1+1;
            Scalar faceInfoClr = BGR_GREEN;
            rectangle(frameOCD, Point(x1, y1), Point(x2, y2), faceInfoClr,2, 4);
            if (validFace(w,h)) {
                time ( &rawtime );
                incSesn(faceCnt);
                ptm = gmtime ( &rawtime );
                pic_t *facePic = new pic_t;
                facePic->caption1 = format("%04d%02d%02d%02d%02d%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday
                    , ptm->tm_hour, ptm->tm_min, ptm->tm_sec);  /* capTstamp */
                facePic->caption2 = format("%d-%d-%dx%d-%d-%d",faceCnt.sesn,faceCnt.idx,w,h,x1,y1); /* capDims */
                
                float sideMargin = faceCaptureMargin/2;
                int wm = w*sideMargin;
                int hm = h*sideMargin;
                Rect faceMask(x1-wm,y1-hm,w+2*wm,h+2*hm);
                if (faceMask.x<0) faceMask.x = 0;
                if (faceMask.y<0) faceMask.y = 0;
                if ( (faceMask.x+faceMask.width) > frameOCD.cols) faceMask.width = frameOCD.cols - faceMask.x;
                if (faceMask.y+faceMask.height > frameOCD.rows) faceMask.height = frameOCD.rows - faceMask.y;
                frameOCD(faceMask).copyTo(facePic->pix);
                facePic->filename = picDepotPath + picPrefix + format("%s-%s", facePic->caption1.c_str(), facePic->caption2.c_str()) + picSuffix;
                cout << "Capture #" << faceCnt.tot << ":" << facePic->caption1 << "-" << facePic->caption2 << endl;
                mq.enq(facePic);
                fDetected = true;
            }
            else {
                faceInfoClr = BGR_YELLOW;
            }
            rectangle(frameOCD, Point(x1, y1), Point(x2, y2), faceInfoClr,2, 4);
            // String dimTxt = format("%d:%dx%d",pBodyDet->getCandidateIdx(), w, h);
            String dimTxt = format("C:%dx%d", w, h);
            int fontFace = FONT_HERSHEY_SIMPLEX;
            int fontScale = 1;
            int thickness = 2;
            int baseline = 0;
            Size textSize = getTextSize(dimTxt, fontFace,fontScale, thickness, &baseline);
            putText(frameOCD, dimTxt, Point(x1, y2+textSize.height+4), fontFace
              , fontScale, faceInfoClr, thickness);
        }
    }
    return fDetected;
}

void processFrame (frame_t *f) {
    string title = getFAInfo();
    // cout << title << ",Zooming in and searching..." << getZoomInTimer() << endl;
    double t = cv::getTickCount();
    if(detectFaceOpenCVDNN(f->frame)){
        double tt_opencvDNN = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
        double fpsOpencvDNN = 1/tt_opencvDNN;
        title += format(", %s;FPS = %.2f,",modelName.c_str(), fpsOpencvDNN);
    }
    else {
        if (faceCnt.idx>1) {
            newSesn(faceCnt);
        }
        title += ", searching...";
    }
    // title += format("%d", getZoomInTimer());
    title += format("%d", f->frameCnt);
    putText(f->frame, title.c_str(), Point(10, 50), FONT_HERSHEY_SIMPLEX, 1.2, BGR_RED, 4);
}

 void processItem(frame_t *f){
    if (-1 == f->frameCnt) {
        cout << "stop in processItem()" << endl;
        fRunLoop = false;
        return;
    }
    // cout << "frmProcThrd:processItem:processFrame:" << f->frameCnt << endl;
    processFrame(f);
    // cout << "frmHandler:" << f->frameCnt << "," << fShowFrm << endl;
    if (fShowFrm){
        // cout << "Thread-" <<  getThrdName() << ": show frame #" << f->frameCnt << endl;
        // imshow(kWinName, f->frame);
    }
}

void scanLoop (string &src) {
    fProcessing = false;
    fRunLoop = true;
    int frameCnt = 0;

    VideoCapture cap;
    cap.open(src);
    // preThreadStart();
    // threadStartNotify();
    do {
        frame_t *i;
        /* pending on Mat Queue */
        // cout << "fifoProcThrd:threadFunc:processItem" << endl;
        // fifoQ->deq(&i, true);
        i = new frame_t;
        cap >> i->frame;
        i->frameCnt = ++frameCnt;
        processItem(i);
        delete i;
        if(0==fifoQ->getQSize()){
            fProcessing = false;    
        }
        if(waitKey(5) >=0) {
            fRunLoop = false;
        }
        // cout << "fifoQ->getQSize()=" << fifoQ->getQSize() << endl;
    }while(fRunLoop || fifoQ->getQSize());
    cout << "fifoProc::threadFunc(): endloop" << endl;
    // postThreadLoop();
}

bool loadNet (int fmt, string &modelPath, string &configPath) {
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
        /*switch(fmt){
            case 0:
                net = cv::dnn::readNetFromCaffe(configPath, modelPath);
                break;
            case 1:
                net = cv::dnn::readNetFromTensorflow(modelPath, configPath);
                break;
            default:
                assert(0);
        }*/
        // net.setPreferableBackend(parser.get<int>("backend"));
        net.setPreferableBackend(DNN_BACKEND_INFERENCE_ENGINE);
        double tLoad = 1000*((double)cv::getTickCount() - tStart)/cv::getTickFrequency();
        cout << format("Load DNN for %s...%.3f mS", appName.c_str(), tLoad) << endl;
        // outNames = getOutputsNames(net);
        // fLoaded = true;
        fNetReady = true;
    }
    return fNetReady;
}

int ftAddToQ(frame_t *f) {
    int qSize;
    qSize=fifoQ->enq(f);
    fProcessing = true;
    cout << "FaceDet:" << qSize << "," << f->frameCnt << endl;
    if(0==f->frameCnt) {
        cout << "Init NN processing for FaceDet..." << endl;
        while(fProcessing){
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
        }
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

bool initVars(int argc, char **argv,frmProcParam_t &FPP, frmFeederParams_t &ffp, faceDetParams_t &fdp) {
    
    int scr_w, scr_h;
    int wndPosX,wndPosY;
    int wndWidth,wndHeight;
    getScreenResolution(scr_w, scr_h);
    wndPosX = scr_w/2;
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
    FPP.wRect = Rect(wndPosX,wndPosY,wndWidth,wndHeight);

    if(!parseFaceDetectArg(argc, argv, (void*)&ffp, fdp)) {
       return false;
    }
    ffp.func = (void *)ftAddToQ;
    scale = fdp.scale;
    inpWidth = fdp.inpWidth;
    inpHeight = fdp.inpHeight;
    confThreshold = fdp.confThreshold;
    blobMean = fdp.mean;
    swapRB = fdp.swapRB;
    framework = fdp.framework;
    wLim = fdp.wLim;
    hLim = fdp.hLim;
    faceCaptureMargin = fdp.faceCaptureMargin;
    if (fdp.framework) {
        modelName = string("TF");
    }else {
        modelName = string("Caffe");
    }

    picDepotPath = ".";
    picPrefix = ffp.outputPrefix;
    picSuffix = ffp.outputSuffix;
    
    resetStat(faceCnt);
    return true;
}

int main(int argc, char** argv)
{

    frmProcParam_t FPP;
    frmFeederParams_t ffp;
    /* model parameters for face detection */
    faceDetParams_t fdp;
    assert(initVars(argc, argv, FPP, ffp, fdp));

    // frmFeederThrd *feeder = new frmFeederThrd(&ffp);
    fifoQ = new mediaQueue<frame_t>;
    thread ppt1(picProcThrd, &mq);

    cout << "Load NN:" << fdp.modelPath << "," << fdp.configPath << endl;
    assert(loadNet(fdp.framework, fdp.modelPath, fdp.configPath));
    net.setPreferableTarget(fdp.compTarget);
    assert(!net.empty());
    
    setZoomInTimer(1);

    namedWindow(FPP.wName, WINDOW_NORMAL);
    moveWindow(FPP.wName, FPP.wRect.x, FPP.wRect.y);
    resizeWindow(FPP.wName, FPP.wRect.width, FPP.wRect.height);
    
    // feeder->run();
    stringstream uri;
    uri << "rtsp://" << ffp.rtsp.user << ":" << ffp.rtsp.pswd << "@" << ffp.rtsp.host;
    string src = uri.str();
    scanLoop(src);

    pic_t *endOne = new pic_t;
    mq.enq(endOne);
    ppt1.join();
    delete fifoQ;
    // delete feeder;
}