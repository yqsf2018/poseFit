#ifndef _APP_INFO
#define _APP_INFO

#include <string>

using namespace std;

typedef struct {
    int sesn;
    int idx;
    int tot;
} stat_t;

/* data structures */

int getZoomInLimit(void);
bool setZoomInLimit(int z);

/* get APIs */
// int get(void);
int getZoomInTimer(void);
// float get(void);
// char const * get(void);
char const * getFAInfo (void);

/* set APIs */
// bool set(int );
bool setZoomInTimer(int z);
bool tickZoomInTimer();
// bool set(float );
// bool set(char const * );
bool setFAInfo(char const * str);

char const * getFaceAreaInfo(void); 
bool setFaceAreaInfo(char const *str );

#endif