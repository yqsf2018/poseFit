#ifndef _BDET_HDR
#define _BDET_HDR

typedef struct {
    int target_w;
    int target_h;
    int nDetected;
    int nPromise;
    int nCandiate;
} overView_t;

typedef struct {
    int candidateIdx;
    float zoomRatio;
    int zoomCmd;
} faceView_t;

#endif