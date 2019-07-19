#ifndef _CURL_EXT
#define _CURL_EXT

typedef struct {
  unsigned int pan;
  unsigned int tilt;
  unsigned int zoom;
  unsigned int zoomVal;
} ptz_t;

typedef struct {
  unsigned int x;
  unsigned int y;
  unsigned int dzoom;
} dpos_t;

typedef struct {
  char const *classTxt;
  unsigned int classTxtLen;
  unsigned int sesnId;
  unsigned int frameId;
  unsigned int left;
  unsigned int top;
  unsigned int width;
  unsigned int height;
  double conf;
  char const *picPath;
  unsigned int picPathLen;
} obj_t;

typedef struct {
  unsigned int sesnId;
  unsigned int instId;
  unsigned int left;
  unsigned int top;
  unsigned int width;
  unsigned int height;
  double conf;
  char const *picPath;
  unsigned int picPathLen;
} face_t;

typedef struct {
    int camID;
    char restUrl[256];
    bool fPost;
    int timeout;
    void *iJsonPtr;
} curlReq_t;

enum cam_type {
  CAMTYPE_DAHUA = 0
  ,CAMTYPE_HIK
  ,CAMTYPE_NUM
};

enum dc_api_t {
  NONE = 0
  ,CMD_BASE = 10
  ,GET_VER = 10
  ,GET_PTZ
  ,SET_PTZ
  ,GO_PRE
  ,SAVE_PRE
  ,GO_POS
  ,ADD_OBJ
  ,ADD_FACE
  ,API_END
};

enum err_t {
  ERR_NONE = 0
  ,ERR_START = 100
  ,ERR_CURL_FAIL_CREATE_HANDLE    /* Failed to create curl handle in fetch_session */
  ,ERR_CURL_FAIL_FETCH_URL        /* Failed to fetch url */
  ,ERR_JSON_FAIL_ALLOC            /* Failed to populate payload */
  ,ERR_JSON_FAIL_PARSE            /* Failed to parse json string */
  ,ERR_CAM_EXEC
  ,ERR_CAM_BUSY
  ,ERR_INVALID_POLL
  ,ERR_INVALID_TOKEN
  ,ERR_THRD_FAIL_CREATE
  ,ERR_THRD_NOT_STARTED
  ,ERR_FAILED_ADD_Q
  ,ERR_PENDING
  ,ERR_END
};

/*const char *getErrMsg(void);
// void doCurl( curlReq_t *req );
int getVer(const char *restAddr, int restPort);
int getPTZ(int cam_id, const char *restAddr, int restPort, int *token);
int setPTZ(int cam_id, const char *restAddr, int restPort, void *dst);

int goPre(int cam_id, const char *restAddr, int restPort, int refId);
int savePre(int cam_id, const char *restAddr, int restPort, int refId);
int goPos(int cam_id, const char *restAddr, int restPort, void *dst);

int getVerResp(int ret, void *rst);
int getPTZResp(int ret, int token, void *rst);
// int dfltRestResp(const char *opStr, int ret, int token);
int checkPendingResp(int *cmd, int *token);
int getResp(int cmd, int token);

#define setPTZResp(ret) (dfltRestResp("SET_PTZ",ret))
#define goPreResp(ret) (dfltRestResp("GO_PRE", ret))
#define savePreResp(ret) (dfltRestResp("SAVE_PRE", ret))
#define goPosResp(ret) (dfltRestResp("GO_POS", ret))

void curlInit(void);
void curlStart(void);
void curlStop(void);*/

#endif //_CURL_EXT