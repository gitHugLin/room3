//
// Created by linqi on 16-4-5.
//

#ifndef GLESUTILS_H
#define GLESUTILS_H

#include <android/log.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include "log.h"
#include <CameraGL.h>
#include <hardware/gralloc.h>

using namespace android;

using android::GraphicBuffer;
using android::sp;


const char gPerspectiveVertexShader[] =
        "attribute vec4 a_position;\n"
                "uniform vec2 textureSize;\n"
                "attribute vec2 a_texCoord;\n"
                "varying vec2 texCoord;\n"
                "void main() {\n"
                "  texCoord = a_texCoord;\n"
                "  gl_Position = a_position;\n"
                "}\n";

const char gPerspectiveFragmentShader[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision highp float;\n"
                "varying vec2 texCoord;\n"
                "uniform samplerExternalOES u_samplerTexture;\n"

                "void main() {\n"
                "  gl_FragColor = texture2D(u_samplerTexture,texCoord);\n"
                //"  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
                "}\n";

class GLESUtils
{
public:
    GLESUtils();
    ~GLESUtils();

public:
    int initOpenGLES(alloc_device_t *m_alloc_dev, int width, int height);
    int Progress(int* targetAddr, int mode);
    bool setMode(int);
	int updateImageData(struct cv_fimc_buffer *m_buffers_capture);
    int GLProcess(int* targetAddr, int mode);
	int getResult(int targetAddr);
private:
	int mCurrentId;
    double work_begin;
    double work_end;
    float mWidth;
    float mHeight;
    bool checkInitOpenGLES;

    GLuint vPositionHandle;
    GLuint vTexCoordHandle;
    GLuint vSizeHandle;

    GLuint programObject;
    // texture
    GLuint textureID1;
    GLuint textureID2;
    GLuint textureID3;
    GLuint textureID4;
    GLuint textureID5;
    GLuint textureID6;

    EGLContext context;
    EGLDisplay display;
	EGLSurface surface;

    GLuint targetTexId;
    GLuint fboTargetHandle;
    EGLImageKHR mTargetEGLImage;
    sp <GraphicBuffer> mTargetGraphicBuffer;
    char* mTargetGraphicBufferAddr;
	buffer_handle_t mTargetBufferHandle;

	class CameraGLTexImage {
	public:
		int mId;
		int mShareFd;
		EGLDisplay eglDisplay;
		char* mBufferAddr;
		buffer_handle_t mHandle;
	    EGLImageKHR mEGLImage;
	    sp <GraphicBuffer> mGraphicBuffer;

		CameraGLTexImage(int id, EGLDisplay display = NULL, char* bufAddr = NULL, buffer_handle_t handle = NULL, int share_fd = 0);
		~CameraGLTexImage();

		int createTexImage(int width, int height, int format);
		int updateTexImage(int width, int height, int format);
		void clear();
		buffer_handle_t bufferHandleAlloc(uint32_t w, uint32_t h, PixelFormat format, uint32_t usage);
		void bufferHandleFree(buffer_handle_t handle);
	};
	CameraGLTexImage* mCameraGLTexImage[6];// = {NULL};
	GLuint mTextureIds[6];

private:
    int InitEGL();
    int DestroyEGL();
    void checkFBO();
    void workBegin();
    void workEnd(char* module_name = "null");
    GLuint LoadShader( GLenum type, const char *shaderSrc );
    GLuint LoadProgram( const char *vShaderStr, const char *fShaderStr );
    void initializeTmpResEGLImage(alloc_device_t *m_alloc_dev, int fboWidth, int fboHeight, GLuint *tex,
                                  GLuint * fbo, GLuint texGroup);
};
#endif //GLESUTILS_H

