#ifndef _OBJ_BOX_FILTER
#define _OBJ_BOX_FILTER

typedef struct objbox {
    unsigned int frameIdx;
    unsigned int rect_left;
    unsigned int rect_top;
    unsigned int rect_width;
    unsigned int rect_height;
    float confScore;
    int clsId;
    int stayDur;
    int leaveDur;
    int objID;

    objbox(unsigned int frmIdx, unsigned int left, unsigned int top, unsigned int w, unsigned int h, float conf, unsigned int cls, int id)
        :frameIdx(std::move(frmIdx)), rect_left(std::move(left)), rect_top(std::move(top)), rect_width(std::move(w)), rect_height(std::move(h))
        ,confScore(std::move(conf)), clsId(std::move(cls)), objID(std::move(id)), stayDur(std::move(1)), leaveDur(std::move(0)) 
        {
        }

    objbox& operator=(const objbox& other) = default;
} objbox_t;

#define FRM_UPDATE_WND (60*25)
#define OBJ_LEAVE_MAX (50)
#define OBJ_STAY_MIN (250)
#define OBJ_STAY_WEIGHT 10000L
#define OBJ_MERGE_DIST_MAX (20)
#define OBJ_MERGE_AREA_DIFF_MAX (100)
#define OBJ_DEL_AFTER_DETECT 1

#define OBJ_LEAVE_MAX_DFLT (50)
#define OBJ_STAY_MIN_DFLT (25)
#define OBJ_MERGE_DIST_DFLT (10)
#define OBJ_MERGE_AREA_DIFF_DFLT (20)

void obj_box_init(void);
int getFusionThrsDist (void);
int getFusionThrsDist2 (void);
void setFusionThrsDist (int s);

int getFusionThrsArea (void);
int getFusionThrsArea2 (void);
void setFusionThrsArea (int s);

int getObjLeaveMax (void);
void setObjLeaveMax (int s);

int getObjStayMin (void);
void setObjStayMin (int s);

void obj_box_clearStay (void) ;
int obj_box_add(unsigned int frmIdx, unsigned int left, unsigned int top, unsigned int w, unsigned int h, float conf, unsigned int cls);
int obj_box_update(unsigned int frmIdx, int &nPromise);
objbox_t *obj_box_getTarget(bool fPop);
objbox_t *obj_box_get(unsigned int idx);

#endif