#ifndef _POST_PROC_PIC
#define _POST_PROC_PIC
#include <string>
// #include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

typedef struct pic {
    cv::Mat pix;
    std::string filename;
    std::string caption1;
    std::string caption2;
} pic_t;

void picProcThrd( void *procCfg );
#endif
