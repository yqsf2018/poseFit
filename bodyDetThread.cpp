#include <iostream>
#include <string>
#include <sstream>
#include <vector>
// #include "captureThread.h"
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include "pubDef.h"
// #include "picQ.hpp"
// #include "bodyDetThread.h"
#include "bodyDetThread.hpp"
// #include "frameProc.hpp"
#include "obj_box_filter.hpp"
#include "dcCtrl.h"
#include "appInfo.h"

using namespace std;
using namespace cv;
using namespace cv::dnn;

void bodyDetThrd::adjFaceAreaWidth(int pos) {
    faceAreaPerc_w = pos * 0.01f;
    fAdjOvTarget = true;
    /* if (fFaceAreaLockRatio) {
        faceAreaPerc_h = faceAreaPerc_w/faRatio;
        int pos1 = (int)(faceAreaPerc_h*100);
        setTrackbarPos(fahTBar,cWinName,pos1);   
    }
    else {
        updateFaRatio();
    }*/
    // cout << "faceAreaPerc_w = " << faceAreaPerc_w << endl;
}

void bodyDetThrd::adjFaceAreaHeight (int pos) {
    faceAreaPerc_h = pos * 0.01f;
    fAdjOvTarget = true;
    /* if (fFaceAreaLockRatio) {
        faceAreaPerc_w = faceAreaPerc_h*faRatio;
        int pos1 = (int)(faceAreaPerc_w*100);
        setTrackbarPos(fawTBar,cWinName,pos1);   
    }
    else {
        updateFaRatio();
    } */
    // cout << "faceAreaPerc_h = " << faceAreaPerc_h << endl;
}

int bodyDetThrd::calcZoomRatio(int view_w, int view_h, int area_w, int area_h, float *zoomRatio) {
    float zw = view_w * faceAreaPerc_w/area_w;
    float zh = view_h * faceAreaPerc_h/area_h;

    if(zw < zh) {
        zh = zw;
    }

    if (zh <= 1 ){
        zoomRatio[0] = 1.0f;
        return 0;
    }
    else if(zh >= 4) {
        zoomRatio[0] = 4.0f;
        return 128;
    }
    else {
        zoomRatio[0] = zh;
        return (int)(128*(zh-1)/3+0.5);    
    }
}

void bodyDetThrd::addCaption( string &capTxt, int top,int left, int fontType, double fontSize, cv::Scalar clr, int thick){
    frameCaption_t cap;

    int baseLine;
    Size labelSize = getTextSize(capTxt, fontType, fontSize, thick, &baseLine);

    if (0==top) {
        cap.bottom = capHeadY + labelSize.height;
        capHeadY = cap.bottom + capLineMarginY;
    }
    else {
        cap.bottom = top + labelSize.height;
    }

    if (0==left) {
        cap.left = capHeadX;
        // capHeadX += labelSize.width + capLineMarginX;
    }
    else {
        cap.left = left;
    }

    cap.attr.typeName = fontType;
    cap.attr.scale = fontSize;
    cap.attr.color = clr;
    cap.attr.thickness = thick;
    cap.txtLine = capTxt;

    capList.push_back(cap);
}

string bodyDetThrd::getObjLabel(float conf,int classId, Rect *rect){
    string label="";
    if  (conf >=0.0) {
        label = format("%.2f", conf);
        if (!classes.empty() ){
            CV_Assert(classId < (int)classes.size());
            if ( ("All"==target) || (classes[classId] == target) ) 
            {
                label = classes[classId] + ": " + label;
            }
            /*else {
                return;
            }*/
        }    
    }
    else {
        assert(rect);
        objbox_t *pOB = obj_box_get((unsigned int)classId);
        assert(pOB);
        if (pOB) {
            rect->x = pOB->rect_left;
            rect->y = pOB->rect_top;
            rect->width = pOB->rect_width;
            rect->height = pOB->rect_height;
            label = format("%d:%d/%d", pOB->objID, pOB->stayDur, pOB->leaveDur);
        }
    }
    return label;
}

void bodyDetThrd::drawPred(string label, int labelPos, Rect &rect, Mat& frame, Scalar &lineColor, int lineThickness)
{
    int left = rect.x;
    int right = rect.x + rect.width;
    int top = rect.y;
    int bottom = rect.y + rect.height;
    
    rectangle(frame, Point(left, top), Point(right, bottom), lineColor, lineThickness);

    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.75, 1, &baseLine);

    top = max(top, labelSize.height);
    int labelH = 0;
    switch(labelPos){
        case 2:
            labelH = (bottom+top)/2;
            break;
        case 1:
            labelH = bottom;    
            break;
        case 0:
        default:
            labelH = top;
    }
    rectangle(frame, Point(left, labelH - labelSize.height),
              Point(left + labelSize.width, labelH + baseLine), Scalar::all(255), FILLED);
    putText(frame, label, Point(left, labelH), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(), 1);
}

void bodyDetThrd::postprocess(Mat& frame, int frameCnt)
{
    static std::vector<int> outLayers = net.getUnconnectedOutLayers();
    static std::string outLayerType = net.getLayer(outLayers[0])->type;

    
    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<Rect> boxes;
    Rect r;
    int classId;
    int oni = net.getLayer(0)->outputNameToIndex("im_info");
    // cout << "net.getLayer(0)->outputNameToIndex()=" << oni
    //    << ",outLayerType=" << outLayerType << endl;
    if ( oni != -1) { // Faster-RCNN or R-FCN
        std::cout << "im_info" << std::endl;
        // Network produces output blob with a shape 1x1xNx7 where N is a number of
        // detections and an every detection is a vector of values
        // [batchId, classId, confidence, left, top, right, bottom]
        CV_Assert(outs.size() == 1);
        float* data = (float*)outs[0].data;
        for (size_t i = 0; i < outs[0].total(); i += 7) {
            float confidence = data[i + 2];
            if (confidence > bodyConfThreshold) {
                classId = (int)(data[i + 1]) - 1;
                r.x = (int)data[i + 3];
                r.y = (int)data[i + 4];
                r.width = (int)data[i + 5];
                r.height = (int)data[i + 6];
                r.width -= r.x - 1;
                r.height -= r.y - 1;
                if (validTarget(classId)) {
                    classIds.push_back(classId);  // Skip 0th background class id.
                    boxes.push_back(r);
                    confidences.push_back(confidence);
                }
            }
        }
    }
    else if (outLayerType == "DetectionOutput") {
        // std::cout << "outLayerType=" << outLayerType << std::endl;
        // Network produces output blob with a shape 1x1xNx7 where N is a number of
        // detections and an every detection is a vector of values
        // [batchId, classId, confidence, left, top, right, bottom]
        CV_Assert(outs.size() == 1);
        float* data = (float*)outs[0].data;
        for (size_t i = 0; i < outs[0].total(); i += 7) {
            float confidence = data[i + 2];
            if (confidence > bodyConfThreshold) {
                classId = (int)(data[i + 1]) - 1;
                r.x = (int)(data[i + 3] * frame.cols);
                r.y = (int)(data[i + 4] * frame.rows);
                r.width = (int)(data[i + 5] * frame.cols);
                r.height = (int)(data[i + 6] * frame.rows);
                r.width -= r.x - 1;
                r.height -= r.y - 1;
                if (validTarget(classId)) {
                    classIds.push_back(classId);  // Skip 0th background class id.
                    boxes.push_back(r);
                    confidences.push_back(confidence);
                }
            }
        }
    }
    else if (outLayerType == "Region")
    {
        // std::cout << "outLayerType=" << outLayerType << std::endl;
        for (size_t i = 0; i < outs.size(); ++i) {
            // Network produces output blob with a shape NxC where N is a number of
            // detected objects and C is a number of classes + 4 where the first 4
            // numbers are [center_x, center_y, width, height]
            float* data = (float*)outs[i].data;
            // cout << "outs[" << i << "].rows=" << outs[i].rows << endl;
            for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols) {
                Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
                Point classIdPoint;
                double confidence;
                minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
                /*if (confidence > 0.01) {
                     cout << "confidence=" << confidence << ",classIdPoint=" << classIdPoint << endl;
                }*/
                if (confidence > bodyConfThreshold) {
                    classId = classIdPoint.x;
                    int centerX = (int)(data[0] * frame.cols);
                    int centerY = (int)(data[1] * frame.rows);
                    r.width = (int)(data[2] * frame.cols);
                    r.height = (int)(data[3] * frame.rows);
                    r.x = centerX - r.width / 2;
                    r.y = centerY - r.height / 2;
                    if (validTarget(classId)) {
                        classIds.push_back(classId);  // Skip 0th background class id.
                        boxes.push_back(r);
                        confidences.push_back(confidence);
                    }
                }
            }
        }
    }
    else
        CV_Error(Error::StsNotImplemented, "Unknown output layer type: " + outLayerType);
    
    std::vector<int> indices;
    string info;
    // cout << "boxes[] = " << boxes.size() << endl;
    NMSBoxes(boxes, confidences, bodyConfThreshold, nmsThreshold, indices);
    Scalar lineClr = BGR_GREEN;
    int lineThick = 2;
    ovInfo.nDetected = indices.size();
    string objLabel;
    Rect labelRect;
    int nHB = indices.size();
    info = format("HB:%d", nHB);
    addCaption(info);    
    for (size_t i = 0; i < nHB; ++i){
            int idx = indices[i];
            Rect box = boxes[idx];
            objLabel = getObjLabel(confidences[idx], classIds[idx], nullptr);
            
            drawPred(objLabel, 0, box, frame, lineClr, lineThick);
            info = format("Raw Obj Detail:%d,%d,%d,%d,%d,%.3f", frameCnt, box.x,box.y,box.width,box.height,confidences[idx]);
            // cout << info << endl;
            obj_box_add(frameCnt, box.x, box.y, box.width, box.height, confidences[idx], classIds[idx]);
        }
    if (nHB>0) {
        cout << frameCnt << "," << info.c_str() << endl;
    }

    ovInfo.nCandiate = obj_box_update(frameCnt, ovInfo.nPromise);
    
    lineClr = BGR_BLUE;
    lineThick = 1;
    for (size_t i = 0; i < ovInfo.nPromise; ++i){
        objLabel = getObjLabel(-1.0,i, &labelRect);
        drawPred(objLabel, 1, labelRect, frame, lineClr, lineThick);
    }
    info = format("Frame #%d: update and re-draw %d p-boxes", frameCnt, ovInfo.nPromise);
    // cout << info << endl;
    // int st = dcUpdateState(zoomCam);
    // if (CAM_ST_OVERVIEW == st)
    vector<obj_t> objList;
    for (int i=0;i<camNum;i++) {
        objbox_t *pOB = obj_box_getTarget(true);
        if (pOB) {
            obj_t obj1;
            obj1.classTxt = classes[pOB->clsId].c_str();
            obj1.classTxtLen = classes[pOB->clsId].length();
            obj1.sesnId = pOB->objID;
            obj1.frameId = pOB->frameIdx;
            obj1.left=pOB->rect_left;
            obj1.top=pOB->rect_top;
            obj1.width=pOB->rect_width;
            obj1.height=pOB->rect_height;
            obj1.conf=pOB->confScore;
            obj1.picPath = NULL;
            obj1.picPathLen = 0;
            objList.push_back(obj1);
            info = format("Target Obj: %d,%d,%d,%d,%d,%d", frameCnt, obj1.sesnId, obj1.left, obj1.top
                , obj1.width,obj1.height);
        }
        else {
            break;
        }
    }
    if (objList.size()) {
        // reportObj((void *)&objList);
    }
}

vector<String> bodyDetThrd::getOutputsNames(void) {
    static vector<String> names;
    if (names.empty()) {
        //Get the indices of the output layers, i.e. the layers with unconnected outputs
        vector<int> outLayers = net.getUnconnectedOutLayers();
        
        //get the names of all the layers in the network
        vector<String> layersNames = net.getLayerNames();
        /*for (int i=0;i<layersNames.size();i++){
            cout << "NN Layer Names:" << layersNames[i] << endl;    
        }*/
        // Get the names of the output layers in names
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i){
            names[i] = layersNames[outLayers[i] - 1];
            //cout << "bodyDetThrd::getOutputsNames(): names[" << i << "]=" << names[i] << endl;
        }
    }
    return names;
}

void bodyDetThrd::processFrame (frame_t *f) {
    // cout << "bodyDetThrd::processFrame():" << f->frameCnt << "," << (int)(f->flags) << "," << CHECK_FLAG(f->flags, FF_MASK_SKIP) << endl;
    /*if (CAM_ST_OVERVIEW != dcGetState()) {
        return;
    }*/
    resetCapLoc();
    if (!CHECK_FLAG(f->flags, FF_MASK_SKIP)) {
        // Create a 4D blob from a frame.
        Size inpSize(inpWidth > 0 ? inpWidth : f->frame.cols,
                     inpHeight > 0 ? inpHeight : f->frame.rows);
        // cout << f->frameCnt << ":inpSize=" << inpSize
        //     << ",f->frame:" << f->frame.rows << "," << f->frame.cols << endl;
        blobFromImage(f->frame, blob, scale, inpSize, b_mean, swapRB, false);
        // Run a model.
        net.setInput(blob);
        if (net.getLayer(0)->outputNameToIndex("im_info") != -1) { // Faster-RCNN or R-FCN 
            cout << "im_info resize" << endl;
            resize(f->frame, f->frame, inpSize);
            Mat imInfo = (Mat_<float>(1, 3) << inpSize.height, inpSize.width, 1.6f);
            net.setInput(imInfo, "im_info");
        }
        std::vector<Mat> cur_outs;
        net.forward(cur_outs, getOutputsNames()); // , this->
        // cout << "bodyDetThrd::Filter Frame with NN," << f->frameCnt << ",Q=" << getQSize() << endl;
        outs = cur_outs;
        std::vector<double> layersTimes;
        double freq = getTickFrequency();
        double tt = net.getPerfProfile(layersTimes) / freq;
        labelTxt = format("%.3f mS, %.2f FPS", 1000*tt, 1/tt);   
        // cout << f->frameCnt << ":"  << "freq="<< freq << ","  << "tt="<< tt << endl;
    }
    // addCaption(labelTxt); 
    if (fAdjOvTarget) {
        updateOvTarget(f->frame);
    }
    string wholeAreaInfo = format("%02d:%02d:%02d:%03d/%d", f->pt[PT_HOUR], f->pt[PT_MIN], f->pt[PT_SEC], f->pt[PT_MSEC], f->frameCnt);
    addCaption(wholeAreaInfo);
    postprocess(f->frame, f->frameCnt);

    // Put efficiency information.
    wholeAreaInfo = format("Fusion(%d,%d)-D:%d,P:%d,C:%d-%s", getFusionThrsDist2(), getFusionThrsArea2(), ovInfo.nDetected, ovInfo.nPromise, ovInfo.nCandiate, labelTxt.c_str());
    // putText(f->frame, wholeAreaInfo.c_str(), Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, BGR_BLUE, 2);
    addCaption(wholeAreaInfo);
    if (fShowFaceTargetRect){
        rectangle(f->frame, Point(f->frame.cols/2 - ovInfo.target_w/2, f->frame.rows/2 - ovInfo.target_h/2)
            , Point(f->frame.cols/2 + ovInfo.target_w/2, f->frame.rows/2 + ovInfo.target_h/2), BGR_YELLOW, 2);    
    }
    /* add captions */
    for (frameCaption_t cap : capList){
        // frameCaption_t cap = capList.back();
        putText(f->frame, cap.txtLine, Point(cap.left, cap.bottom), cap.attr.typeName, cap.attr.scale, cap.attr.color, cap.attr.thickness);    
        // capList.pop_back();
    }
    capList.clear();
}