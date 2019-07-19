#ifndef _DC_CTRL
#define _DC_CTRL

#include "curlExt.h"

typedef struct {
    int camIdx;
    int presetIdx;
} camCfg_t;

bool dcInit(int zinLimit, int camId);

void dcAddCfg(int camId, int presetIdx);

int dcGetState(void);

int dcUpdateState(int zoomCam);

bool dcZoomIn(int zoomCam, void *posPtr);

bool dcZoomOut(int zoomCam, int presetIdx);

int reportObj(void *objInfo);

void dcEnd(void);

#endif