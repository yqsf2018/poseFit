#ifndef _CAP_THREAD
#define _CAP_THREAD

typedef struct capThrdCfg {
    const char* vidSrcStr;
    void *bodyDetThrdPtr;
    void *faceDetThrdPtr;
 } capThrdCfg_t;


int getSkipLimit(int kind);
void setSkipLimit(int kind, int limit);
void capThread(capThrdCfg_t *pCtc);
bool isCapRunning(void)
#endif