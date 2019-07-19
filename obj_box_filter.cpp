#include <iostream>
#include <sstream>
#include <cstdio>
#include <cmath>
#include <vector>
#include <queue>
#include "pubDef.h"
#include "obj_box_filter.hpp"

using namespace std;

#define GET_CNTR(point, side) ((point) + (side)/2)

vector<objbox_t> obSet;
int objIdIdx = 0;
int objStayMin = OBJ_STAY_MIN;
int objLeaveMax = OBJ_LEAVE_MAX;
int mergeThrsDist = OBJ_MERGE_DIST_MAX;
int mergeThrsDist2 = OBJ_MERGE_DIST_MAX*OBJ_MERGE_DIST_MAX;
int mergeThrsArea = OBJ_MERGE_AREA_DIFF_MAX;
int mergeThrsArea2 = OBJ_MERGE_AREA_DIFF_MAX*OBJ_MERGE_AREA_DIFF_MAX;


long calcWeight (objbox_t &b) {
    long w = - (long)(b.confScore*b.rect_width*b.rect_height);
    w -= b.stayDur * OBJ_STAY_WEIGHT;
    return w;
}

struct cmpBox
{
    bool operator()(const int& a, const int& b)
    {
        return calcWeight(obSet[a]) > calcWeight(obSet[b]);
    }
};

priority_queue<int, std::vector<int>, cmpBox> sbox;

int getFusionThrsDist (void) {
    return mergeThrsDist;
}

int getFusionThrsDist2 (void) {
    return mergeThrsDist2;
}

void setFusionThrsDist (int s) {
    mergeThrsDist = s;
    mergeThrsDist2 = mergeThrsDist * mergeThrsDist;
}

int getFusionThrsArea (void) {
    return mergeThrsArea;
}

int getFusionThrsArea2 (void) {
    return mergeThrsArea2;
}

void setFusionThrsArea (int s) {
    mergeThrsArea = s;
    mergeThrsArea2 = mergeThrsArea*mergeThrsArea;
}

int getObjLeaveMax (void) {
    return objLeaveMax;
}

void setObjLeaveMax (int s) {
    objLeaveMax = s;
}

int getObjStayMin (void) {
    return objStayMin;
}

void setObjStayMin (int s) {
    objStayMin = s;
}

void obj_box_clearStay (void) {
    for (int i=0;i<obSet.size();i++) {
        obSet[i].stayDur = 0;
    }
}

void obj_box_init(void) {
    setFusionThrsDist(OBJ_MERGE_DIST_DFLT);
    setFusionThrsArea(OBJ_MERGE_AREA_DIFF_DFLT);
    setObjStayMin(OBJ_STAY_MIN_DFLT/FPS);
    setObjLeaveMax(OBJ_LEAVE_MAX_DFLT/FPS);
}

bool mergeCheck(const objbox_t& b, unsigned int left, unsigned int top, unsigned int w, unsigned int h, int cls) {
    if (b.clsId != cls) {
        return false;
    }
    int cx1 = GET_CNTR(b.rect_left, b.rect_width);
    int cy1 = GET_CNTR(b.rect_top, b.rect_height);;
    int cx2 = GET_CNTR(left, w);
    int cy2 = GET_CNTR(top, h);

    int d2 = (cx2-cx1)*(cx2-cx1) + (cy2-cy1)*(cy2-cy1);

    int a2 = abs(b.rect_width*b.rect_height - w*h*1L);

    // cout << ">>>>> d2=" << d2 << ";a2=" << a2 << endl;

    return ((d2<mergeThrsDist) && (a2<mergeThrsArea2));
}

bool isAheadOf(unsigned int cur, unsigned int prev ) {
    unsigned int diff = cur-prev;

    return ((diff>0) || (diff<FRM_UPDATE_WND));
}

int obj_box_add(unsigned int frmIdx, unsigned int left, unsigned int top, unsigned int w, unsigned int h, float conf, unsigned int cls){
    // printf("%d:%d_%d-%dx%d Check\n", frmIdx, left, top, w, h);
    for (auto b=obSet.begin(); b!=obSet.end();b++ ) {
        // printf("----%d:%d_%d-%dx%d\n", b->frameIdx, b->rect_left, b->rect_top, b->rect_width,b->rect_height);
        /* only update stay duration for boxes in a new frame */
        if ( isAheadOf(frmIdx, b->frameIdx) && mergeCheck(*b, left, top, w, h, cls) ) {
            stringstream info;
            info << "merged: " << b->objID;
            int r1 = b->rect_left + b->rect_width - 1;
            int r2 = left + w - 1;
            int b1 = b->rect_top + b->rect_height - 1;
            int b2 = top + h - 1;
            if(left < b->rect_left) {
                b->rect_left = left;
                info << ", expand to left";
            }
            if(r2 > r1) {
                b->rect_width = r2 - b->rect_left + 1;
                info << ", expand width";
            }
            if(top < b->rect_top) {
                b->rect_top = top;
                info << ", expand to top";   
            }
            if(b2 > b1) {
                b->rect_height = b2 - b->rect_top + 1;
                info << ", expand height";
            }
            b->frameIdx = frmIdx;
            b->stayDur += 1;
            b->leaveDur = 0;
            // cout << info.str() << endl;
            /* return after merge */
            return obSet.size();
        }
    }
    obSet.emplace_back(frmIdx, left, top, w, h, conf, cls, ++objIdIdx);
    cout << "nb" << objIdIdx << " ";
    return obSet.size();
}

bool emptyExpired(const objbox_t& b) {
    return b.leaveDur>objLeaveMax;
}

int obj_box_update(unsigned int frmIdx, int &nPromise) {
    for (auto it=obSet.begin(); it!=obSet.end(); ) {
        if(isAheadOf(frmIdx, it->frameIdx)) {
            // printf("%d:%d_%d-%dx%d leave\n", it->frameIdx, it->rect_left, it->rect_top
            // , it->rect_width,it->rect_height);
            it->leaveDur++;
            it->frameIdx = frmIdx;
        }
        if (emptyExpired(*it)) {
            cout << "eb" << it->objID << " ";
            it = obSet.erase(it);
        } else {
            ++it;
        }
    }
    cout << endl;
    nPromise = obSet.size();

    while(!sbox.empty()) {
        sbox.pop();
    }
    for (int i=0;i<obSet.size();i++) {
        if (obSet[i].stayDur>objStayMin) {
            sbox.push(i);    
        }
    }
    return sbox.size();
}

objbox_t *obj_box_getTarget(bool fPop){
    if (sbox.empty()) {
        return NULL;
    }
    int i = sbox.top();
    if (fPop) {
        sbox.pop();
    }
    obSet[i].stayDur=0;
    return &obSet[i];
}

objbox_t *obj_box_get(unsigned int i) {
    if(obSet.empty() || (i>=obSet.size()) ) {
        return NULL;
    }
    return &obSet[i];
}