#ifndef _ARG_PARSER
#define _ARG_PARSER

#include <string>
#include <opencv2/imgproc.hpp>

#define SKIP_FRAME (0)
#define WND_POSX_DFLT (100)
#define WND_POSY_DFLT (100)

#define FACE_CAP_MRGN_DFLT (0.2)
#define WND_WIDTH_MIN (400)
#define WND_HEIGHT_MIN (300)
#define WND_WH_RATIO (16.0/9.0)
#define PRESET_DFLT (10)
#define ZOOM_IN_SEC_MAX (5)
#define ZOOM_IN_FRM_MAX (FPS * ZOOM_IN_SEC_MAX)

#define CAM_PRESET_DFLT (10)


using namespace std;
using namespace cv;

typedef struct {
    float bodyConfThreshold;
    float nmsThreshold;
    float scale;
    Scalar b_mean;
    bool swapRB;
    int inpWidth;
    int inpHeight;
    std::string classPath;
    std::string modelPath;
    std::string configPath;
    std::string target;
    int compTarget;
} bodyDetParams_t;

typedef struct {
    float scale;
    int inpWidth;
    int inpHeight;
    float confThreshold;
    Scalar mean;
    bool swapRB;
    int framework;
    int wLim;
    int hLim;
    std::string modelPath;
    std::string configPath;
    float faceCaptureMargin;
    int compTarget;
} faceDetParams_t;

bool parseBodyDetectArg(int argc, char **argv, void *frmFeederParamPtr, bodyDetParams_t& bdp);

bool parseFaceDetectArg(int argc, char **argv, void *frmFeederParamPtr, faceDetParams_t& fdp);

#endif