#ifndef _FIFO_PROC
#define _FIFO_PROC

#include <cassert>
#include <thread>
#include <iostream>
#include <string>
#include <sstream>
#include "picQ.hpp"

using namespace std;

template<class T>
class fifoProcThrd {
private:
    std::thread _t;
    bool fNewThread;
    bool fRunThread;
    bool fTerminated;
    bool fProcessing;
    string thrdName;
    mediaQueue<T> *fifoQ;
    
    void threadFunc (void) {
        fTerminated = false;
        fProcessing = false;
        preThreadStart();
        threadStartNotify();
        do {
            T *i;
            /* pending on Mat Queue */
            // cout << "fifoProcThrd:threadFunc:processItem" << endl;
            fifoQ->deq(&i, true);
            processItem(i);
            delete i;
            if(0==fifoQ->getQSize()){
                fProcessing = false;    
            }
            // cout << "fifoQ->getQSize()=" << fifoQ->getQSize() << endl;
        }while(fRunThread || fifoQ->getQSize());
        cout << "fifoProc::threadFunc(): endloop" << endl;
        postThreadLoop();
        fTerminated = true;
    }
public:
    char const * getThrdName(void) {
        return thrdName.c_str();
    }
    fifoProcThrd(string &name) : _t() {
        thrdName = name;
        fRunThread = true;
        fNewThread = false;
        fifoQ = new mediaQueue<T>;
        cout<< "fifoProcThrd constructor: create instance " << thrdName << endl;
    }
    ~fifoProcThrd() {
        if (fNewThread) {
            cout<< "fifoProcThrd de-constructor: joinable=" << _t.joinable() << endl;
            // fRunThread = false;
            if (_t.joinable()>0) {
                // cout<< "fifoProcThrd de-constructor: about to join" << endl;
                _t.join();
                // cout<< "fifoProcThrd de-constructor: join finish" << endl;
                threadEndNotify();
                // cout<< "fifoProcThrd de-constructor: wait join to finish " << endl;
            }
        }
        delete fifoQ; 
    }

    virtual int AddToQ (T *i) {
        if (fRunThread){
            fProcessing = true;
            return fifoQ->enq(i);
        }
        else {
            return -1;
        }
    }

    bool isDone(void) {return fTerminated;}

    virtual void preThreadStart (void) {
    }
    virtual void postThreadLoop (void) {
    }
    virtual void threadStartNotify (void) {
        stringstream info;
        info << "Thread " << thrdName << " started" << endl;
        cout<< info.str() << endl;
    }
    virtual void threadEndNotify (void) {
        stringstream info;
        info << "Thread " << thrdName.c_str() << " ended" << endl;
        cout<< info.str() << endl;
    }
    virtual void processItem(T *i) { }
    
    virtual void stop(void){
        // cout << "Stopping thread loop" << endl;
        fRunThread = false;
    }

    bool isBusy(void) {
        return fProcessing;
    }
    
    int getQSize(void) {
        return fifoQ->getQSize();
    }

    void run( bool fNewThrd ) {
        fNewThread = fNewThrd;
        // cout<< "fifoProcThrd.run(): kick off thread" << endl;
        if (fNewThread){
            _t = std::thread(&fifoProcThrd::threadFunc, this);
        }
        else {
            threadFunc();
        }

    }
};

#endif