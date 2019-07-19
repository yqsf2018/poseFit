#ifndef _FRM_FEEDER_H
#define _FRM_FEEDER_H
#include <string>
#include <opencv2/imgproc.hpp>
#include "pubDef.h"

using namespace std;
using namespace cv;

typedef struct {
    string host;
    int port;
    string user;
    string pswd;
} rstpCfg_t;

typedef struct {
    int skipLimit;
    int qSizeLimit;
    bool fLocalSrc;
    int fps;
    rstpCfg_t rtsp;
    string srcPath;
    string outputPrefix;
    string outputSuffix;
    void *func;
} frmFeederParams_t;

void setSourceInfo(frmFeederParams_t *ffp);

void setBlock(bool fBlock);

void feedThread(void *pCfg);

#endif