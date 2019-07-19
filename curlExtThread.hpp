#ifndef _CUR_EXT_THRD
#define _CUR_EXT_THRD

#include <iostream>
/* json-c (https://github.com/json-c/json-c) */
#include <json-c/json.h>
#include "curlExt.h"
#include "fifoProc.hpp"
using namespace std;

extern int doCurl( curlReq_t *req, string & );

class curlProcThrd : public fifoProcThrd<curlReq_t> {
private:
    bool fInQuery;
    string host;
    int port;
    int eCode;
    string eMsg;
    int curlToken;
    int curToken;
    int curlQuery(char const * fmt, int fPost, json_object *ijson, int timeout, int *token);
    int dfltRestResp(const char *opStr, int ret, int token);
public:
    curlProcThrd(string name, char const *restAddr, int restPort) : fifoProcThrd<curlReq_t>(name) {
        host = string(restAddr);
        port = restPort;
        curlToken=0;
        curToken=0;
        cout<< "curlProcThrd constructor: create instance " << this->getThrdName() << "for host " << host << ":" << port << endl;
    }
    ~curlProcThrd(){
        cout<< "curlProcThrd deconstructor: destroy instance " << this->getThrdName() << endl;
    }

    void setHost(char const *addr) {
        host = addr;
    }
    void setPort(int p) {
        port = p;
    }

    const char *getErrMsg(void) {
      return eMsg.c_str();
    }

    virtual void preThreadStart (void){
        cout << "curlProcThrd::preThreadStart()" << endl;
    }

    virtual void postThreadLoop (void){
        cout << "curlProcThrd::postThreadLoop()" << endl;
    }

    virtual void threadStartNotify (void){
        stringstream info;
        info << "Curl Thread " << this->getThrdName() << " started" << endl;
        cout<< info.str() << endl;
    }

    virtual void threadEndNotify (void){
        stringstream info;
        info << "Curl Thread " << this->getThrdName() << " ended" << endl;
        cout<< info.str() << endl;
    }

    virtual void processItem(curlReq_t *i){
        // cout << "curlProcThrd::processItem():" << i->camID << "," << i->restPort << endl;
        if (-1 == i->camID) {
            cout << "stop in curlProcThrd::processItem()" << endl;
            this->stop();
        }
        else {
            eCode = doCurl(i, eMsg);
        }
    }

    int getBEVer(int camType = CAMTYPE_DAHUA);
    int getBEVerResp(int ret, void *rst);

    int readPTZ(int cam_id, int *token, int = CAMTYPE_DAHUA);
    int getPTZResp(int ret, int token, void *rst);

    int checkPendingResp(int *cmd, int *token);
    int getResp(int cmd, int token);

#define setPTZResp(ret) (dfltRestResp("SET_PTZ",ret))
#define goPreResp(ret) (dfltRestResp("GO_PRE", ret))
#define savePreResp(ret) (dfltRestResp("SAVE_PRE", ret))
#define goPosResp(ret) (dfltRestResp("GO_POS", ret))

    int goPreset(int cam_id, int presetId, int = CAMTYPE_DAHUA);

    int movePTZ(int cam_id, void *ptz, int = CAMTYPE_DAHUA);

    int savePreset(int cam_id, int presetId, int = CAMTYPE_DAHUA);

    int gotoPos(int cam_id, void *pos, int = CAMTYPE_DAHUA);

    int addObj(void *objData, int camType);

    int addPerson(void *objData, int camType);

    int addFace(void *faceData, int camType);
    /*virtual void stop(){
        cout << "curlProcThrd::stop()" << endl;
    }*/
};
#endif