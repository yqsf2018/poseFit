#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>

#include "common.hpp"

std::string keys =
    "{ help  h     | | Print help message. }"
    // "{ @alias      | | An alias name of model to extract preprocessing parameters from models.yml file. }"
    // "{ zoo         | models.yml | An optional path to file with preprocessing parameters }"
    "{ device      |  0 | camera device number. }"
    "{ input i     | | Path to input image or video file. Skip this argument to capture frames from a camera. }"
    "{ model m     | 0 | Optional name of an origin framework of the model. Detect it automatically if it does not set. }"
    // "{ classes     | | Optional path to a text file with names of classes to label detected objects. }"
    "{ thr         | .5 | Confidence threshold. }"
    //"{ nms         | .4 | Non-maximum suppression threshold. }"
    //"{ dTarget         | All | target object. }"
    // "{ backend     |  0 | Choose one of computation backends: "
    //                     "0: automatically (by default), "
    //                     "1: Halide language (http://halide-lang.org/), "
    //                     "2: Intel's Deep Learning Inference Engine (https://software.intel.com/openvino-toolkit), "
    //                     "3: OpenCV implementation }"
    "{ iscale s   | 1.0 | input frame scale factor. }"
    "{ iwidth x   | 300 | input frame width. }"
    "{ iheight y  | 300 | input frame height. }"
    "{ target      | 0 | Choose one of target computation devices: "
                         "0: CPU target (by default), "
                         "1: OpenCL, "
                         "2: OpenCL fp16 (half-float precision), "
                         "3: VPU }";

using namespace cv;
using namespace std;
using namespace cv::dnn;

size_t inWidth = 300;
size_t inHeight = 300;
double inScaleFactor = 1.0;
float confidenceThreshold = 0.5;
cv::Scalar meanVal(104.0, 177.0, 123.0);
int nnModel = 0;
std::string modelName;
int nTarget = 0;
std::string inputSrc ="dflt";

// #define CAFFE

const std::string caffeConfigFile = "/home/tinoq/fitPose/obj/opencv_face/deploy.prototxt";
const std::string caffeWeightFile = "/home/tinoq/fitPose/obj/opencv_face/res10_300x300_ssd_iter_140000_fp16.caffemodel";

const std::string tensorflowConfigFile = "/home/tinoq/fitPose/obj/opencv_face/opencv_face_detector.pbtxt";
const std::string tensorflowWeightFile = "/home/tinoq/fitPose/obj/opencv_face/opencv_face_detector_uint8.pb";

void detectFaceOpenCVDNN(Net net, Mat &frameOpenCVDNN)
{
    int frameHeight = frameOpenCVDNN.rows;
    int frameWidth = frameOpenCVDNN.cols;
    cv::Mat inputBlob;
    switch(nnModel) {
      case 1:
        inputBlob = cv::dnn::blobFromImage(frameOpenCVDNN, inScaleFactor, cv::Size(inWidth, inHeight), meanVal, true, false);
        break;
      case 0:
      default:
        inputBlob = cv::dnn::blobFromImage(frameOpenCVDNN, inScaleFactor, cv::Size(inWidth, inHeight), meanVal, false, false);
    }

    net.setInput(inputBlob, "data");
    cv::Mat detection = net.forward("detection_out");

    cv::Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

    for(int i = 0; i < detectionMat.rows; i++)
    {
        float confidence = detectionMat.at<float>(i, 2);

        if(confidence > confidenceThreshold)
        {
            int x1 = static_cast<int>(detectionMat.at<float>(i, 3) * frameWidth);
            int y1 = static_cast<int>(detectionMat.at<float>(i, 4) * frameHeight);
            int x2 = static_cast<int>(detectionMat.at<float>(i, 5) * frameWidth);
            int y2 = static_cast<int>(detectionMat.at<float>(i, 6) * frameHeight);
            int w = x2-x1;
            int h = y2-y1;
            String dimTxt = format("%dx%d",w, h);
            int fontFace = FONT_HERSHEY_SIMPLEX;
            int fontScale = 1;
            int thickness = 2;
            int baseline = 0;
            Size textSize = getTextSize(dimTxt, fontFace,fontScale, thickness, &baseline);
            putText(frameOpenCVDNN, dimTxt, Point(x1, y2+textSize.height+4), fontFace
              , fontScale, Scalar(0, 255, 0), thickness);
            cv::rectangle(frameOpenCVDNN, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 0),2, 4);
        }
    }
}

int main( int argc, const char** argv )
{

  CommandLineParser parser(argc, argv, keys);
  parser = CommandLineParser(argc, argv, keys);
  parser.about("Use this script to run face detection DNN using OpenCV.");
  if (argc == 1 || parser.has("help")) {
      parser.printMessage();
      return 0;
  }
  if (parser.has("thr")) {
    confidenceThreshold = parser.get<float>("thr");  
  }
  if (parser.has("model")) {
    nnModel = parser.get<int>("model");  
  }
  if (parser.has("iscale")) {
    inScaleFactor = parser.get<float>("iscale");  
  }
  if (parser.has("iwidth")) {
    inWidth = parser.get<float>("iwidth");  
  }
  if (parser.has("iheight")) {
    inHeight = parser.get<float>("iheight");  
  }
  if (parser.has("target")) {
    nTarget = parser.get<int>("target");  
  }
  if (parser.has("input")) {
    inputSrc = parser.get<String>("input");  
  }
  
  // nmsThreshold = parser.get<float>("nms");
  // Scalar mean = parser.get<Scalar>("mean");
  // bool swapRB = parser.get<bool>("rgb");
  Net net;
  switch(nnModel) {
    case 1:
      modelName = "TF";
      net = cv::dnn::readNetFromTensorflow(tensorflowWeightFile, tensorflowConfigFile);
      break;
    case 0:
    default:
      modelName = "CAFFE";
      net = cv::dnn::readNetFromCaffe(caffeConfigFile, caffeWeightFile);
  }
  net.setPreferableTarget(nTarget);
  // net.setPreferableBackend(0);
  cout << "DNN Model:" << modelName 
       << "\nConfThrs:" << confidenceThreshold
       << "\nTarget:" << nTarget
       << "\nInput Source:" << inputSrc
       << "\nInput Scale Factor:" << inScaleFactor
       << "\nInput Width:" << inWidth 
       << "\nInput Height:" << inHeight
       << endl;

  VideoCapture source;
  if ("dflt" == inputSrc) {
    source.open(0);
  }
  else {
    source.open(inputSrc);
  }
      
  Mat frame;

  double tt_opencvDNN = 0;
  double fpsOpencvDNN = 0;
  
  while(1)
  {
      source >> frame;
      if(frame.empty())
          break;
      double t = cv::getTickCount();
      detectFaceOpenCVDNN ( net, frame );
      tt_opencvDNN = ((double)cv::getTickCount() - t)/cv::getTickFrequency();
      fpsOpencvDNN = 1/tt_opencvDNN;
      putText(frame, format("OpenCV DNN %s;FPS = %.2f",modelName.c_str(), fpsOpencvDNN), Point(10, 50), FONT_HERSHEY_SIMPLEX, 1.4, Scalar(0, 0, 255), 4);
      imshow( "OpenCV - DNN Face Detection", frame );
      int k = waitKey(5);
      if(k == 27)
      {
        destroyAllWindows();
        break;
      }
    }
}