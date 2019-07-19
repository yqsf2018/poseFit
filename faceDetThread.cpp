#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <thread>
// #include "captureThread.h"
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include "pubDef.h"
#include "appInfo.h"
// #include "picQ.hpp"
// #include "faceDetThread.h"
#include "postProcPic.hpp"
#include "faceDetThread.hpp"
using namespace std;
using namespace cv;
using namespace dnn;

static const std::string picDepotPath = "./pic/";
static const std::string picPrefix = "fp_";
static const std::string picSuffix = ".jpg";

time_t rawtime;
struct tm * ptm = NULL;

mediaQueue<pic_t> mq;
thread *ppt1;

void faceDetThrd::initPicThread(void) {
    pic_t *endOne = new pic_t;
    cout << "init picProcThrd" << endl;
    ppt1 = new thread(picProcThrd, &mq);
}

void faceDetThrd::endPicThread(void) {
    pic_t *endOne = new pic_t;
    cout << "Stop picProcThrd" << endl;
    mq.enq(endOne);
    ppt1->join();
    delete ppt1;
}

int faceDetThrd::detectFaceOpenCVDNN(Mat &frameOCD)
{
    bool fDetected = false;

    int frameHeight = frameOCD.rows;
    int frameWidth = frameOCD.cols;
    Size inpSize(inpWidth > 0 ? inpWidth : frameOCD.cols,
                inpHeight > 0 ? inpHeight : frameOCD.rows);
    Mat inputBlob = cv::dnn::blobFromImage(frameOCD,scale, inpSize, mean, swapRB, framework);
    
    net.setInput(inputBlob, "data");
    cv::Mat detection = net.forward("detection_out");

    cv::Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());
    // cout << "detectionMat.rows:" << detectionMat.rows << endl;
    for(int i = 0; i < detectionMat.rows; i++)
    {
        float confidence = detectionMat.at<float>(i, 2);

        if(confidence > confThreshold) {
            int x1 = static_cast<int>(detectionMat.at<float>(i, 3) * frameWidth);
            int y1 = static_cast<int>(detectionMat.at<float>(i, 4) * frameHeight);
            int x2 = static_cast<int>(detectionMat.at<float>(i, 5) * frameWidth);
            int y2 = static_cast<int>(detectionMat.at<float>(i, 6) * frameHeight);
            int w = x2-x1+1;
            int h = y2-y1+1;
            Scalar faceInfoClr = BGR_GREEN;
            if (fFaceRect) {
                rectangle(frameOCD, Point(x1, y1), Point(x2, y2), faceInfoClr,2, 4);
            }
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
            if (!fFaceRect) {
                rectangle(frameOCD, Point(x1, y1), Point(x2, y2), faceInfoClr,2, 4);
            }
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

void faceDetThrd::processFrame (frame_t *f) {
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
    title += format("%d", getZoomInTimer());
    putText(f->frame, title.c_str(), Point(10, 50), FONT_HERSHEY_SIMPLEX, 1.2, BGR_RED, 4);
}