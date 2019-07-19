#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
// #include <cstdarg>
#include <cmath>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>

/* json-c (https://github.com/json-c/json-c) */
#include <json-c/json.h>
/* libcurl (http://curl.haxx.se/libcurl/c) */
#include <curl/curl.h>
#include "pubDef.h"
#include "curlExt.h"
#include "curlExtThread.hpp"

#define SKIP_PEER_VERIFICATION
#define SKIP_HOSTNAME_VERIFICATION

#define CURLOPT_TO_MAX 30
#define CURLOPT_TO_RST 15
#define CURLOPT_TO_AVG 5

using namespace std;

/* holder for curl fetch */
struct curl_fetch_st {
    char *payload;
    size_t size;
};

typedef struct curl_cfg {
    char url[256];
    unsigned int timeout;
    int fPost;
    string ijStr;
} curl_cfg_t;

char invalidResp[] = "INVALID_RESP";

char const *opName[] = {
  "SET_PTZ"
  ,"GO_PRE"
  ,"SAVE_PRE"
  ,"GO_POS"
};

bool fExec = false;
int apiInQuery;
bool fSuccTransfer = true;
json_object *ojson;
string postBuf;

/* callback for curl fetch */
size_t curl_callback (void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;                             /* calculate buffer size */
    struct curl_fetch_st *p = (struct curl_fetch_st *) userp;   /* cast pointer to fetch struct */

    /* expand buffer */
    p->payload = (char *) realloc(p->payload, p->size + realsize + 1);

    /* check buffer */
    if (p->payload == NULL) {
      /* this isn't good */
      fprintf(stderr, "ERROR: Failed to expand buffer in curl_callback");
      /* free buffer */
      free(p->payload);
      fSuccTransfer = false;
      /* return */
      return -1;
    }

    /* copy contents to buffer */
    memcpy(&(p->payload[p->size]), contents, realsize);

    /* set new buffer size */
    p->size += realsize;

    /* ensure null termination */
    p->payload[p->size] = 0;

    /* return size */
    return realsize;
}

/* fetch and return url body via curl */
CURLcode curl_fetch_url(CURL *ch, const char *url, struct curl_fetch_st *fetch, unsigned int timeout) {
    CURLcode rcode;                   /* curl result code */

    /* init payload */
    fetch->payload = (char *) calloc(1, sizeof(fetch->payload));

    /* check payload */
    if (fetch->payload == NULL) {
        /* log error */
        fprintf(stderr, "ERROR: Failed to allocate payload in curl_fetch_url");
        /* return error */
        return CURLE_FAILED_INIT;
    }

    if (timeout > CURLOPT_TO_MAX) {
      timeout = CURLOPT_TO_MAX;
    }

    /* init size */
    fetch->size = 0;

    /* set url to fetch */
    curl_easy_setopt(ch, CURLOPT_URL, url);

    /* set calback function */
    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, curl_callback);

    /* pass fetch struct pointer */
    curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) fetch);

    /* set default user agent */
    curl_easy_setopt(ch, CURLOPT_USERAGENT, "fitpose/1.0");

    /* set timeout */
    curl_easy_setopt(ch, CURLOPT_TIMEOUT, timeout);

    /* enable location redirects */
    curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);

    /* set maximum allowed redirects */
    curl_easy_setopt(ch, CURLOPT_MAXREDIRS, 1);
    #ifdef SKIP_PEER_VERIFICATION
    /*
     * If you want to connect to a site who isn't using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */ 
    curl_easy_setopt(ch, CURLOPT_SSL_VERIFYPEER, 0L);
    #endif
 
    #ifdef SKIP_HOSTNAME_VERIFICATION
    /*
     * If the site you're connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */ 
    curl_easy_setopt(ch, CURLOPT_SSL_VERIFYHOST, 0L);
    #endif

    fSuccTransfer = true;
    /* fetch the url */
    rcode = curl_easy_perform(ch);

    /* return */
    return rcode;
}

void clearInputJson (int apiEnum, json_object *jsn) {
    switch(apiEnum) {
    case ADD_OBJ:
    case ADD_FACE:
        json_object *jField;
        if (json_object_object_get_ex(jsn, "total", &jField)){
            int objCnt = json_object_get_int(jField);
            json_object *objList;
            if(json_object_object_get_ex(jsn, "list", &objList)) {
                while(objCnt--){
                     json_object_put(json_object_array_get_idx(objList, objCnt));
                }
                json_object_put(objList);
            }
        }
        break; 
    default:
        break;
    }
    json_object_put(jsn);
}

int doCurl( curlReq_t *req, string& errMsg) 
{
    CURL *ch;
    struct curl_slist *headers = NULL;                      /* http headers to send with request */  
    CURLcode rcode;                                         /* curl result code */
    enum json_tokener_error jerr = json_tokener_success;    /* json parse error */
    struct curl_fetch_st curl_fetch;                        /* curl fetch struct */
    struct curl_fetch_st *cf = &curl_fetch;                 /* pointer to fetch struct */
    errMsg = "None";
    int errCode=ERR_NONE;

    cout << "doCurl(): start " 
        << req->restUrl << "," 
        << req->timeout 
        << endl;
    do{
         /* url to test site */
        // char *url = "http://jsonplaceholder.typicode.com/posts/";

        /* init curl handle */
        if ((ch = curl_easy_init()) == NULL) {
            /* log error */
            errMsg = "ERROR: Failed to create curl handle in fetch_session";
            fprintf(stderr, errMsg.c_str());
            /* return error */
            errCode = ERR_CURL_FAIL_CREATE_HANDLE;
            break;
        }
        /* set content type */
        headers = curl_slist_append(headers, "Accept: application/json");
        json_object *ijson;

        if (req->fPost) {
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(ch, CURLOPT_CUSTOMREQUEST, "POST");
            // curl_easy_setopt(ch, CURLOPT_POSTFIELDS, json_object_to_json_string(ijson));
            assert(req->iJsonPtr);
            ijson = (json_object *)(req->iJsonPtr);
            postBuf = json_object_to_json_string(ijson);
            cout << "ijStr=" << postBuf.c_str() << endl;
            curl_easy_setopt(ch, CURLOPT_POSTFIELDS, postBuf.c_str());
            curl_easy_setopt(ch, CURLOPT_POSTFIELDSIZE, postBuf.length());
        }
        else { 
            curl_easy_setopt(ch, CURLOPT_HTTPGET, 1L); 
        }
        curl_easy_setopt(ch, CURLOPT_HTTPHEADER, headers);

        rcode = curl_fetch_url(ch, req->restUrl, cf, req->timeout);
        /* cleanup curl handle */
        curl_easy_cleanup(ch);

        /* free headers */
        curl_slist_free_all(headers);
        if (req->fPost) {
            json_object *jField;
            int apiEnum = -1;
            if (json_object_object_get_ex(ijson, "_api", &jField)) {
                apiEnum = json_object_get_int(jField);
            }
            clearInputJson(apiEnum, ijson);
        }

    /* check return code */
    
        if (rcode != CURLE_OK || cf->size < 1) {
            /* log error */
            errMsg = curl_easy_strerror(rcode);
            fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s\n",
                req->restUrl, errMsg.c_str());
            /* return error */
            // apiInQuery = NONE;
            errCode = ERR_CURL_FAIL_FETCH_URL;
            break;
        }

        /* check payload */
        if (cf->payload != NULL) {
            /* print result */
            // printf("CURL Returned: \n%s\n", cf->payload);
            /* parse return */
            ojson = json_tokener_parse_verbose(cf->payload, &jerr);
            /* free payload */
            free(cf->payload);
        } else {
            /* error */
            fprintf(stderr, "ERROR: Failed to populate payload");
            /* free payload */
            free(cf->payload);
            /* return */
            errCode = ERR_JSON_FAIL_ALLOC;
            break;   
        }

        /* check error */
        if (jerr != json_tokener_success) {
            /* error */
            fprintf(stderr, "ERROR: Failed to parse json string");
            /* free json object */
            json_object_put(ojson);
            ojson = NULL;
            /* return */
            errCode = ERR_JSON_FAIL_PARSE;
            break;
        }

        /* debugging */
        // printf("Parsed JSON: %s\n", json_object_to_json_string(ojson[0]));

        /* exit */
        errCode = ERR_NONE;
    }while(0);
    cout << "doCurl(): end," << errCode << "," << errMsg << endl;
    fExec = false;
    return errCode;
}

int curlProcThrd::curlQuery(char const * fmt, int fPost, json_object *ijson, int timeout, int *token) {
    if (0!=curToken) {
        return ERR_CAM_BUSY;
    }
    curToken = ++curlToken;
    curlReq_t *req = new curlReq_t;
    int urlLen = sprintf(req->restUrl, fmt, host.c_str(), port);
    req->camID = 0;
    req->restUrl[urlLen] = 0;
    req->iJsonPtr = (void *)ijson;
    req->fPost = fPost;
    req->timeout = timeout;
    int ret = AddToQ(req);
    if (ret<0) {
        return ERR_THRD_NOT_STARTED;
    }
    else if (0==ret) {
        return ERR_FAILED_ADD_Q;
    }
    else {
        fExec = true;
        if (token) {
            token[0] = curToken;    
        }
        return ERR_PENDING;
    }
}

char const *dhFmtStrs[] = {
    "https://%s:%d/dc/ver"
    ,"https://%s:%d/dc/ptz-get"

    ,"https://%s:%d/dc/ptz-set"
    ,"https://%s:%d/dc/preset-go"
    ,"https://%s:%d/dc/preset-save"
    ,"https://%s:%d/dc/pos-go"
};

char const * getCamRestFmt(int camType, int cmd) {
    switch(camType) {
        case CAMTYPE_DAHUA:
            return dhFmtStrs[cmd];
        case CAMTYPE_HIK:
        default:
            assert(0);
    }
}

int curlProcThrd::getBEVer(int camType) {
    return curlQuery(getCamRestFmt(camType, GET_VER-CMD_BASE), 0, NULL, CURLOPT_TO_AVG, nullptr);
}

int curlProcThrd::getBEVerResp(int ret, void *rst) {
    assert(NULL!=rst);
    if (fExec) {
        return ERR_PENDING;
    }
    if (ERR_PENDING == ret) {
        ret = eCode;
    }
    curToken = 0;
    if (ERR_NONE == ret) {
        char *ver = (char *)rst;
        json_object *detail;
        if(json_object_object_get_ex(ojson, "detail", &detail)) {
            int verLen = json_object_get_string_len(detail);
            memcpy(ver, json_object_get_string(detail), verLen);
            ver[verLen] = 0;
        }
        else {
            ver = invalidResp;
            // ver[verLen] = 0;
        }
    }
    /* free json object */
    json_object_put(ojson);
    return ret;
}

int curlProcThrd::readPTZ(int cam_id, int *token, int camType) {
    json_object *ij;
    ij = json_object_new_object();
    json_object_object_add(ij, "cam_id", json_object_new_int(cam_id));
    return curlQuery(getCamRestFmt(camType, GET_PTZ-CMD_BASE), 1, ij, CURLOPT_TO_AVG, token);
}

int curlProcThrd::getPTZResp(int ret, int token, void *rst) {
    assert(NULL!=rst);
    if (fExec) {
        return ERR_PENDING;
    }
    if (0==curToken) {
        return ERR_INVALID_POLL;
    }
    if (token!=curToken) {
        return ERR_INVALID_TOKEN;
    }
    curToken = 0;
    if (ERR_PENDING == ret) {
        ret = eCode;
    }
    if (ERR_NONE == ret) {
        do {
            ptz_t *ptz = (ptz_t *)rst;
            ptz->pan = ptz->tilt = ptz->zoom = ptz->zoomVal = 0;
            json_object *detail;
            if (json_object_object_get_ex(ojson, "detail", &detail)){
                json_object *jField;
                if (json_object_object_get_ex(detail, "pan", &jField)){
                    ptz->pan = json_object_get_int(jField);        
                }
                if (json_object_object_get_ex(detail, "tilt", &jField)){
                    ptz->tilt = json_object_get_int(jField);        
                }
                if (json_object_object_get_ex(detail, "zoom", &jField)){
                    ptz->zoom = json_object_get_int(jField);        
                }
                if (json_object_object_get_ex(detail, "zoomValue", &jField)){
                    ptz->zoomVal = json_object_get_int(jField);        
                }
            }
        }while(0);
    }
    else {
        assert(0);
    }
    /* free json object */
    json_object_put(ojson);
    return ret;
}

int curlProcThrd::movePTZ(int cam_id, void *dst, int camType) {
    json_object *ij = json_object_new_object();
    ptz_t *ptz = (ptz_t *)dst;
    json_object_object_add(ij, "cam_id", json_object_new_int(cam_id));
    json_object_object_add(ij, "pan", json_object_new_int(ptz->pan));
    json_object_object_add(ij, "tilt", json_object_new_int(ptz->tilt));
    json_object_object_add(ij, "zoom", json_object_new_int(ptz->zoom));
    apiInQuery = SET_PTZ;

    return curlQuery(getCamRestFmt(camType, SET_PTZ-CMD_BASE), 1, ij, CURLOPT_TO_AVG, nullptr);
    // return curlQuery("https://%s:%d/77abc56f-c161-4d8d-8943-adea7d73ac27", restAddr, restPort, 1, ij, CURLOPT_TO_AVG, nullptr);
}

int curlProcThrd::goPreset(int cam_id, int refId, int camType) {
    json_object *ij = json_object_new_object();

    json_object_object_add(ij, "cam_id", json_object_new_int(cam_id));
    json_object_object_add(ij, "ref_id", json_object_new_int(refId));
    apiInQuery = GO_PRE;

    return curlQuery(getCamRestFmt(camType, GO_PRE-CMD_BASE), 1, ij, CURLOPT_TO_AVG, nullptr);
}

int curlProcThrd::savePreset(int cam_id, int refId, int camType) {
    json_object *ij = json_object_new_object();

    json_object_object_add(ij, "cam_id", json_object_new_int(cam_id));
    json_object_object_add(ij, "ref_id", json_object_new_int(refId));
    apiInQuery = SAVE_PRE;

    return curlQuery(getCamRestFmt(camType, SAVE_PRE-CMD_BASE), 1, ij, CURLOPT_TO_AVG, nullptr);
}

int curlProcThrd::gotoPos(int cam_id, void *dst, int camType) {
    json_object *ij = json_object_new_object();
    dpos_t *dpos = (dpos_t *)dst;

    json_object_object_add(ij, "cam_id", json_object_new_int(cam_id));
    json_object_object_add(ij, "pos_x", json_object_new_int(dpos->x));
    json_object_object_add(ij, "pos_y", json_object_new_int(dpos->y));
    json_object_object_add(ij, "zoom_in", json_object_new_int(dpos->dzoom));
    apiInQuery = GO_POS;

    return curlQuery(getCamRestFmt(camType, GO_POS-CMD_BASE), 1, ij, CURLOPT_TO_AVG, nullptr);
}

int curlProcThrd::addObj(void *objData, int camType){
    json_object *ij = json_object_new_object();
    json_object *iList = json_object_new_array();
    
    vector<obj_t> *pOL = (vector<obj_t> *)objData;
    
    json_object_object_add(ij, "total", json_object_new_int(pOL->size()));

    for(vector<obj_t>::iterator it=pOL->begin(); it!= pOL->end(); it++) {
        json_object *iRect = json_object_new_object();
        json_object_object_add(iRect, "left", json_object_new_int(it->left));
        json_object_object_add(iRect, "top", json_object_new_int(it->top));
        json_object_object_add(iRect, "width", json_object_new_int(it->width));
        json_object_object_add(iRect, "height", json_object_new_int(it->height));
        json_object *iObj = json_object_new_object(); 
        json_object_object_add(iObj, "class", json_object_new_string_len(it->classTxt, it->classTxtLen));
        json_object_object_add(iObj, "confidence", json_object_new_double(it->conf));
        json_object_object_add(iObj, "sesnId", json_object_new_int(it->sesnId));
        json_object_object_add(iObj, "sesnId", json_object_new_int(it->frameId));
        json_object_object_add(iObj, "rect", iRect);
        json_object_object_add(iObj, "picPath", json_object_new_string_len(it->picPath, it->picPathLen));
        json_object_array_add(iList, iObj);
    }
    json_object_object_add(ij, "list", iList);
    json_object_object_add(ij, "_api", json_object_new_int(ADD_OBJ));
    apiInQuery = ADD_OBJ;

    return curlQuery(getCamRestFmt(camType, apiInQuery-CMD_BASE), 1, ij, CURLOPT_TO_AVG, nullptr);
}

int curlProcThrd::addPerson(void *objData, int camType){
    // json_object *ij = json_object_new_object();
    obj_t *obj = (obj_t *)objData;

    string classTag("person");
    obj->classTxt = classTag.c_str();
    obj->classTxtLen = classTag.length();
    return addObj(objData, camType);
}

int curlProcThrd::addFace(void *faceData, int camType){
    json_object *ij = json_object_new_object();
    json_object *iList = json_object_new_array();
    
    vector<face_t> *pFL = (vector<face_t> *)faceData;
    
    json_object_object_add(ij, "total", json_object_new_int(pFL->size()));

    for(vector<face_t>::iterator it=pFL->begin(); it!= pFL->end(); it++) {
        json_object *iRect = json_object_new_object();
        json_object_object_add(iRect, "left", json_object_new_int(it->left));
        json_object_object_add(iRect, "top", json_object_new_int(it->top));
        json_object_object_add(iRect, "width", json_object_new_int(it->width));
        json_object_object_add(iRect, "height", json_object_new_int(it->height));
        json_object *iObj = json_object_new_object(); 
        json_object_object_add(iObj, "sesnId", json_object_new_int(it->sesnId));
        json_object_object_add(iObj, "instId", json_object_new_int(it->instId));
        json_object_object_add(iObj, "confidence", json_object_new_double(it->conf));
        json_object_object_add(iObj, "rect", iRect);
        json_object_object_add(iObj, "picPath", json_object_new_string_len(it->picPath, it->picPathLen));
        json_object_array_add(iList, iObj);
    }
    json_object_object_add(ij, "list", iList);
    json_object_object_add(ij, "_api", json_object_new_int(ADD_FACE));

    apiInQuery = ADD_FACE;

    return curlQuery(getCamRestFmt(camType, apiInQuery-CMD_BASE), 1, ij, CURLOPT_TO_AVG, nullptr);
}


int curlProcThrd::checkPendingResp(int *cmd, int *token) {
    if (fExec) {
        return ERR_PENDING;
    }
    cmd[0]=apiInQuery;
    token[0] = curToken;
    return ERR_NONE;
}

const char* validateRestReturn (json_object *oj) {
    json_object *jField;
    if (json_object_object_get_ex(oj, "detail", &jField)) {
        const char *rst = json_object_get_string(jField);
        return (rst[0]=='O' && rst[1]=='K')?NULL:rst; 
    }
    else {
        return invalidResp;
    }
}

int curlProcThrd::dfltRestResp(const char *opStr, int ret, int token) {
    if (fExec) {
        return ERR_PENDING;
    }
    if (0==curToken) {
        return ERR_INVALID_POLL;
    }
    if (token!=curToken) {
        return ERR_INVALID_TOKEN;
    }
    curToken = 0;
    if (ERR_PENDING == ret) {
        ret = eCode;
        if (ERR_NONE == ret) {
            const char * rst = validateRestReturn(ojson);
            if (rst) {
                ret = ERR_CAM_EXEC;
                cout << opStr << " returns error:" << rst << endl;
            }
        }
    }
    else {
        assert(0);
    }
    /* free json object */
    json_object_put(ojson);
    curToken = 0;
    return ret;
}

int curlProcThrd::getResp(int cmd, int token) {
    return dfltRestResp(opName[cmd - CMD_BASE], ERR_PENDING, token);
}