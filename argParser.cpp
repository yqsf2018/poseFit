#include <iostream>
#include <string>
#include "argParser.h"
#include "common.hpp"
#include <opencv2/core/utility.hpp>
#include "frmFeeder.h"

using namespace std;
using namespace cv;

std::string bdpKeys =
    "{ help  h     | | Print help message. }"
    "{ @alias      | | An alias name of model to extract preprocessing parameters from models.yml file. }"
    "{ zoo         | models.yml | An optional path to file with preprocessing parameters }"
    "{ device      |  0 | camera device number. }"
    "{ iqmax q     |  50 | max input queue size. }"
    "{ fps r       |  25 | frames per second. }"
    "{ input i     | | Path to input image or video file. Skip this argument to capture frames from a camera. }"
    "{ rtsp_host       | | RTSP host. }"
    "{ rtsp_port       | 0 | RTSP port }"
    "{ rtsp_user       | | RTSP user }"
    "{ rtsp_pswd       | | RTSP password }"
    "{ src_path       | | RTSP password }"
    "{ o_prefix       | pic/fdet | RTSP password }"
    "{ o_suffix       | .jpg | RTSP password }"
    
    "{ framework f | | Optional name of an origin framework of the model. Detect it automatically if it does not set. }"
    "{ dTarget         | All | target object. }"
    "{ dcWnd d     | 10 | target object. }"
    //"{ classes     | | Optional path to a text file with names of classes to label detected objects. }"
    "{ thr         | .5 | Confidence threshold. }"
    "{ nms         | .4 | Non-maximum suppression threshold. }"
    "{ backend     |  0 | Choose one of computation backends: "
                         "0: automatically (by default), "
                         "1: Halide language (http://halide-lang.org/), "
                         "2: Intel's Deep Learning Inference Engine (https://software.intel.com/openvino-toolkit), "
                         "3: OpenCV implementation }"
    "{ target      | 0 | Choose one of target computation devices: "
                         "0: CPU target (by default), "
                         "1: OpenCL, "
                         "2: OpenCL fp16 (half-float precision), "
                         "3: VPU }";

std::string fdpKeys =
    "{ help  h     | | Print help message. }"
    "{ @alias      | | An alias name of model to extract preprocessing parameters from models.yml file. }"
    "{ zoo         | models.yml | An optional path to file with preprocessing parameters }"
    "{ device      |  0 | camera device number. }"
    "{ iqmax q     |  50 | max input queue size. }"
    "{ fps r       |  25 | frames per second. }"
    "{ input i     | | Path to input image or video file. Skip this argument to capture frames from a camera. }"
    "{ rtsp_host       | | RTSP host. }"
    "{ rtsp_port       | 0 | RTSP port }"
    "{ rtsp_user       | | RTSP user }"
    "{ rtsp_pswd       | | RTSP password }"
    "{ src_path       | | RTSP password }"
    "{ o_prefix       | pic/fdet | RTSP password }"
    "{ o_suffix       | .jpg | RTSP password }"
    
    
    "{ framework f | | Optional name of an origin framework of the model. Detect it automatically if it does not set. }"
    "{ dTarget         | All | target object. }"
    "{ dcWnd d     | 10 | target object. }"
    //"{ classes     | | Optional path to a text file with names of classes to label detected objects. }"
    "{ nms         | .4 | Non-maximum suppression threshold. }"
    "{ f_thr         | .5 | Confidence threshold. }"
    "{ f_iscale s   | 1.0 | input frame scale factor. }"
    "{ f_iwidth x   | 300 | input frame width. }"
    "{ f_iheight y  | 300 | input frame height. }"
    "{ f_faceLimW w  | 100 | input frame height. }"
    "{ f_faceLimH h  | 100 | input frame height. }"
    "{ backend     |  0 | Choose one of computation backends: "
                         "0: automatically (by default), "
                         "1: Halide language (http://halide-lang.org/), "
                         "2: Intel's Deep Learning Inference Engine (https://software.intel.com/openvino-toolkit), "
                         "3: OpenCV implementation }"
    "{ target      | 0 | Choose one of target computation devices: "
                         "0: CPU target (by default), "
                         "1: OpenCL, "
                         "2: OpenCL fp16 (half-float precision), "
                         "3: VPU }";

bool parseForFrmFeeder(CommandLineParser &parser, frmFeederParams_t *fpp) {
    do{
        fpp->skipLimit = parser.get<int>("dcWnd");
        if (fpp->skipLimit<=0) {
            fpp->skipLimit = 1;
        }

        if(parser.has("rtsp_host")){
            fpp->rtsp.host = parser.get<string>("rtsp_host");
            if(parser.has("rtsp_port")) {
                fpp->rtsp.port = parser.get<int>("rtsp_port");
            }
            else {
                fpp->rtsp.port = 0;   
            }
            if(parser.has("rtsp_user") && parser.has("rtsp_pswd")) {
                fpp->rtsp.user = parser.get<string>("rtsp_user");
                fpp->rtsp.pswd = parser.get<string>("rtsp_pswd");    
            }
            else {
                fpp->rtsp.user=string("");
                fpp->rtsp.pswd=string("");   
            }
            fpp->fLocalSrc = false;
        }
        else if (parser.has("input")){
            fpp->srcPath = parser.get<string>("input");
            fpp->fLocalSrc = true;
            fpp->rtsp.host = "None";
            fpp->rtsp.port = 0;
            fpp->rtsp.user = "None";
            fpp->rtsp.pswd = "None";
        }
        else {
            break;
        }

        // if(parser.has("o_prefix"))
        {
            fpp->outputPrefix = parser.get<string>("o_prefix");   
        }
        // if(parser.has("o_suffix"))
        {
            fpp->outputSuffix = parser.get<string>("o_suffix");   
        }
        //if(parser.has("iqmax"))
        {
            fpp->qSizeLimit = parser.get<int>("iqmax");
        }
        
        // if(parser.has("fps"))
        {
            fpp->fps = parser.get<int>("fps");
        }

        cout << "Frame Source:"
             << "\n DC:1/" << fpp->skipLimit
             << "\n Max frame backlog:" << fpp->qSizeLimit
             << "\n Frames Per Second:" << fpp->fps
             << "\n RTSP LocalSrc:" << fpp->fLocalSrc
             << "\n RTSP srcPath:" << fpp->srcPath
             << "\n RTSP Host:" << fpp->rtsp.host
             << "\n RTSP Port:" << fpp->rtsp.port
             << "\n RTSP User:" << fpp->rtsp.user
             << "\n RTSP Pswd:" << fpp->rtsp.pswd
             << "\n Output Prefix:" << fpp->outputPrefix
             << "\n Output Suffix:" << fpp->outputSuffix
             << std::endl;
        return true;
    } while(0);
    return false;
}

bool parseBodyDetectArg(int argc, char **argv, void *frmFeederParamPtr, bodyDetParams_t& BDP){
    CommandLineParser parser(argc, argv, bdpKeys);
    const std::string zooFile = parser.get<String>("zoo");
    const std::string objModels = parser.get<String>("@alias");
    std::string delimiter = ",";
    size_t last = 0, next = 0; 
    int index = 0;
    std::string objModel[2]; 
    while (index<2) { 
        next = objModels.find(delimiter, last); 
        objModel[index++] = objModels.substr(last, next - last);  
        last = next + 1; 
    }

    // return 0;
    /* attach parameters from model configuration file */ 
    bdpKeys += genPreprocArguments(objModel[0], objModel[1], zooFile);
    // cout << "keys=" << keys << endl;

    do {
        // cout << "keys=" << keys << endl;
        parser = CommandLineParser(argc, argv, bdpKeys);
        parser.about("For object detection deep learning networks using OpenCV.");
        if (argc == 1 || parser.has("help")) {
            parser.printMessage();
            break;
        }
        if(parser.has("target")) {
            BDP.compTarget = parser.get<int>("target");    
        }
        else {
            BDP.compTarget = 0;   
        }

        cout << "General Settings:"
             << "\n zooFile=" << zooFile
             << "\n bodyModel=" << objModel[0]
             // << "\n faceModel=" << objModel[1] 
             << "\n compTarget=" << BDP.compTarget
             << std::endl;
        if (!parseForFrmFeeder(parser, (frmFeederParams_t *)frmFeederParamPtr)) {
            break;
        }

        // bodyDetParams_t BDP;
        /* model parameters for body detection */
        if (!parser.has("model")) {
            break;
        }
        BDP.modelPath = findFile(parser.get<String>("model"));
        // BDP.nnModelPath = modelPath.c_str();
        BDP.configPath = findFile(parser.get<String>("config"));
        // BDP.nnConfigPath = configPath.c_str();
        BDP.bodyConfThreshold = parser.get<float>("thr");
        BDP.nmsThreshold = parser.get<float>("nms");
        BDP.scale = parser.get<float>("scale");
        BDP.b_mean = parser.get<Scalar>("mean");
        BDP.swapRB = parser.get<bool>("rgb");
        BDP.inpWidth = parser.get<int>("width");
        BDP.inpHeight = parser.get<int>("height");
        // Open file with classes names.
        BDP.classPath = parser.get<String>("classes");
        if(parser.has("dTarget")) {
            std::cout << "hasTarget=" << parser.has("dTarget") << std::endl;
            BDP.target = parser.get<String>("dTarget");
        }
        cout << "Body Detection Settings:"
             << "\n modelPath=" << BDP.modelPath
             << "\n configPath=" << BDP.configPath
             << "\n bodyConfThreshold=" << BDP.bodyConfThreshold
             << "\n nmsThreshold=" << BDP.nmsThreshold
             << "\n scale=" << BDP.scale
             << "\n b_mean=" << BDP.b_mean
             << "\n swapRB=" << BDP.swapRB
             << "\n inpWidth=" << BDP.inpWidth
             << "\n inpHeight=" << BDP.inpHeight
             << "\n dTarget=" << BDP.target
             << "\n classPath=" << BDP.classPath
             << std::endl;
        return true;
    }while (0);
    return false;
}

bool parseFaceDetectArg(int argc, char **argv, void *frmFeederParamPtr, faceDetParams_t& fdp){
    CommandLineParser parser(argc, argv, bdpKeys);
    const std::string zooFile = parser.get<String>("zoo");
    const std::string objModels = parser.get<String>("@alias");
    std::string delimiter = ",";
    size_t last = 0, next = 0; 
    int index = 0;
    std::string objModel[2]; 
    while (index<2) { 
        next = objModels.find(delimiter, last); 
        objModel[index++] = objModels.substr(last, next - last);  
        last = next + 1; 
    }

    // return 0;
    /* attach parameters from model configuration file */ 
    fdpKeys += genPreprocArguments(objModel[0], objModel[1], zooFile);
    // cout << "keys=" << keys << endl;

    do {
        parser = CommandLineParser(argc, argv, fdpKeys);

        parser.about("For face detection deep learning networks using OpenCV.");
        if (argc == 1 || parser.has("help")) {
            parser.printMessage();
            break;
        }
        if(parser.has("target")) {
            fdp.compTarget = parser.get<int>("target");    
        }
        else {
            fdp.compTarget = 0;   
        }
        
        cout << "General Settings:"
             << "\n zooFile=" << zooFile
             // << "\n bodyModel=" << objModel[0]
             << "\n faceModel=" << objModel[1] 
             << "\n compTarget=" << fdp.compTarget
             << std::endl;
        if (!parseForFrmFeeder(parser, (frmFeederParams_t *)frmFeederParamPtr)) {
            break;
        }

        /* model parameters for face detection */
        // faceDetParams_t fdp;
        if(!parser.has("f_model")){
            break;
        }
        fdp.modelPath = findFile(parser.get<String>("f_model"));
        fdp.configPath = findFile(parser.get<String>("f_config"));
        fdp.confThreshold = parser.get<float>("f_thr");
        fdp.scale = parser.get<float>("f_scale");
        fdp.inpWidth = parser.get<int>("f_width");
        fdp.inpHeight = parser.get<int>("f_height");
        fdp.mean = parser.get<Scalar>("f_mean");
        fdp.swapRB = parser.get<bool>("f_rgb");
        fdp.framework = parser.get<int>("f_framework");
        fdp.faceCaptureMargin = FACE_CAP_MRGN_DFLT;
        if(parser.has("f_faceLimW")){
            fdp.wLim = parser.get<int>("f_faceLimW");
        }
        if(parser.has("f_faceLimH")){
            fdp.hLim = parser.get<int>("f_faceLimH");
        }

        cout << "Face Detection Settings:"
             << "\n faceFramework=" << fdp.framework
             << "\n modelPath=" << fdp.modelPath
             << "\n configPath=" << fdp.configPath
             << "\n faceConfThreshold=" << fdp.confThreshold
             << "\n faceScale=" << fdp.scale
             << "\n f_mean=" << fdp.mean
             << "\n f_swapRB=" << fdp.swapRB
             << "\n f_framework=" << fdp.framework
             << "\n faceInpWidth=" << fdp.inpWidth
             << "\n faceInpWidth=" << fdp.inpHeight
             << "\n faceWidthLimit=" << fdp.wLim
             << "\n faceHeightWidth=" << fdp.hLim
             << std::endl;
        return true;
    } while(0);
    return false;
}