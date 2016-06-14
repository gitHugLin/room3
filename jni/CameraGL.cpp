#include "CameraGL.h"
#include <android/log.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "log.h"
#include "GLESUtils.h"
#include <time.h>

using namespace std;
using namespace android;

static double work_begin = 0;
static double work_end = 0;
static double gTime = 0;

static unsigned long getTickCount() {  
    struct timespec ts;  
    clock_gettime(CLOCK_MONOTONIC, &ts);  
  
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);  
}


static void workBegin()
{
    work_begin = getTickCount();
}

static void workEnd()
{
    work_end = getTickCount() - work_begin;
    //gTime = work_end /((double)getTickFrequency() )* 1000.0;
    LOGE("TIME = %lf ms \n", work_end);
}

static GLESUtils* g_APUnit;

CameraGL::CameraGL():initialized(false)
{
}

CameraGL::~CameraGL()
{
	initialized = false;

}

void CameraGL::init(alloc_device_t *m_alloc_dev, int width, int height)
{
    g_APUnit = new GLESUtils();
    g_APUnit->initOpenGLES(m_alloc_dev, width, height);

	initialized = true;
}

void CameraGL::update(struct cv_fimc_buffer *m_buffers_capture) {
	g_APUnit->updateImageData(m_buffers_capture);
}


long  CameraGL::process(int* targetAddr, int mode)
{
	if (!initialized) {
		LOGE("CameraGL process failed - check init first.");
		return NULL;
	}
   
	workBegin();
    g_APUnit->Progress(targetAddr, mode);

    workEnd();
    return 0;

}

void CameraGL::destroy() {
	delete g_APUnit;
	initialized = false;
}

