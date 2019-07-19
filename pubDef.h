#ifndef _PUB_DEF
#define _PUB_DEF
#include "opencv2/imgcodecs.hpp"

enum pt_t {
    PT_MSEC = 0
    ,PT_SEC
    ,PT_MIN
    ,PT_HOUR
    ,PT_NUM
    ,PT_SIZE  
};

typedef struct frame {
    cv::Mat frame;
    unsigned int pt[PT_NUM];
    int frameCnt;
    char flags;
 } frame_t;

enum cam_st {
    CAM_ST_OVERVIEW = 0,
    CAM_ST_MOVING,
    CAM_ST_ZOMMIN,
    CAM_ST_NUM
};

enum frame_flag {
    FF_SKIP = 0
    ,FF_BLOCK
    ,FF_RECORD
    ,FF_NUM
};

#define FAW_DFLT (0.36)
#define FAH_DFLT (0.60)

#define FF_MASK_SKIP (1<<FF_SKIP)
#define FF_MASK_BLOCK (1<<FF_BLOCK)
#define FF_MASK_RECORD (1<<FF_RECORD)

#define CHECK_FLAG(X,FLAGS)  ( ( (X)&(FLAGS) )>0 )

#define BGR_YELLOW (Scalar(49,247,254))
#define BGR_RED (Scalar(0,0,255))
#define BGR_GREEN (Scalar(0,255,0))
#define BGR_BLUE (Scalar(255,0,0))
#define BGR_L_GREEN (Scalar(56,213,111))

#define SLEEP_INTERVAL 100U   /* 100 mS */

#define FPS (25)

#define CPU_CORE_NUM 8
unsigned int getCamState(void);
bool setCamState(unsigned int st);

#endif