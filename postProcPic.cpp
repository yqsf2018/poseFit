#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include "opencv2/imgproc.hpp"
#include <opencv2/highgui.hpp>
#include "picQ.hpp"
#include "postProcPic.hpp"

using namespace std;
using namespace cv;

#define BGR_BLACK (Scalar(0,0,0))
#define BGR_D_GRAY (Scalar(185,185,185))
#define BGR_GREEN (Scalar(0,255,0))
#define MARGIN_X (0)
#define MARGIN_Y (3)

void picProcThrd( void *procCfg )
{
    assert(procCfg);
    mediaQueue<pic_t> *mq = (mediaQueue<pic_t> *)procCfg;
    cout << "picProcThrd(): start " 
        // << procCfg->url << "," 
        // << procCfg->timeout 
        << endl;
    int fontFace = FONT_HERSHEY_SIMPLEX;
    int thickness = 1;
    int baseline = 0;
    int borderType = BORDER_CONSTANT;
    bool fRunning = true;
    do {
        pic_t *item;
        /* pending on Mat Queue */
        if(false == mq->deq(&item, false)) {
            continue;
        }
        if (item->caption1.empty() || item->filename.empty() ) {
            cout << "picProcThrd:empty Q" << endl;
            fRunning = false;
        }
        else {
            /* get a new mat */
            Mat dst;
            double fontScale = 1.0;
            Size textSize1 = getTextSize(item->caption1, fontFace,fontScale, thickness, &baseline);
            Size textSize2 = getTextSize(item->caption2, fontFace,fontScale, thickness, &baseline);
            // cout << "textSize=" << textSize1 << textSize2 << endl;
            double capWidth = (double)( (textSize1.width > textSize2.width)? textSize1.width : textSize2.width);
            fontScale = item->pix.cols/capWidth;    
            
            int lineWidth = (int)(textSize1.height*fontScale + MARGIN_Y + 0.5); 
            copyMakeBorder( item->pix, dst, 0, lineWidth*2, 0, 0, borderType, BGR_D_GRAY );
            putText(dst, item->caption1, Point(MARGIN_X, dst.rows-lineWidth-1), fontFace
                  , fontScale, BGR_BLACK, thickness);
            putText(dst, item->caption2, Point(MARGIN_X, dst.rows-1), fontFace
                  , fontScale, BGR_BLACK, thickness);
            bool result = false;
            do {
                try{
                    result = imwrite(item->filename, dst);
                    if (result) {
                        cout << "Saved file " << item->filename << endl;
                    }
                    else {
                        cout << "Failed to saved file " << item->filename << endl;   
                    }
                }
                catch (const cv::Exception& ex) {
                    fprintf(stderr, "Exception storing image: %s\n", ex.what());
                }
            }while(false == result);
        }
        delete item;
    }while(fRunning);
    cout << "picProcThrd(): end " 
        // << procCfg->url << "," 
        // << procCfg->timeout 
        << endl;
}