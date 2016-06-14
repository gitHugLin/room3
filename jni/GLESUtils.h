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
                "uniform float nonCurve[17];"
                "attribute vec2 a_texCoord;\n"
                "varying vec2 texCoord;\n"
                "varying float nonlCurve[17];"
                "void main() {\n"
                "	for(int i = 0; i < 17; i++) {\n"
                "  nonlCurve[i] =  nonCurve[i];}\n"
                "  texCoord = a_texCoord;\n"
                "  gl_Position = a_position;\n"
                "}\n";
/*
const char gPerspectiveFragmentShader[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision highp float;\n"
                "varying vec2 texCoord;\n"
                "uniform samplerExternalOES u_samplerTexture;\n"

                "void main() {\n"
                "  gl_FragColor = texture2D(u_samplerTexture,texCoord);\n"
                "}\n";
       
  const char gPerspectiveFragmentShader[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision highp float;\n"
                "varying vec2 texCoord;\n"
                "uniform samplerExternalOES u_samplerTexture;\n"
				"float nonlCurve[17] = {0.0,887.0,1360.0,1714.0,2007.0,2261.0,2489.0,2697.0,2889.0,3069.0,3237.0,3397.0,3549.0,3694.0,3833.0,3967.0,4096.0};\n"
				"float nonlinearCurveLut(float lumiVal) {\n"
				"lumiVal = lumiVal*255.0;\n"
				"float cumXVal = 0.0;\n"
				"float xDeltN = 0.0;\n"
				"float interpWeight = 0.0;\n"
				"float cumYLeft = 0.0, cumYRight = 0.0;\n"
				"float newLumi = 0.0;\n"
				"for(int i = 0; i < 16; i++) {\n"
				"cumXVal += 256.0;\n"
				"if (lumiVal < cumXVal) {\n"
				"cumYLeft = nonlCurve[i];\n"
				"cumYRight = nonlCurve[i+1];\n"
				"interpWeight = cumXVal - lumiVal;\n"
				"newLumi = interpWeight*cumYLeft + (xDeltN - interpWeight)*cumYRight;\n"
				"newLumi = (newLumi + 128.0)>> 8;\n"
				"newLumi = newLumi>>2;\n"
				"newLumi = newLumi<<2;\n"
				"break;}}\n"
				"return newLumi/255.0;}\n"
				"#define MAX(a,b,c) (a>b?(a>c?a:c):(b>c?b:c))\n"
                "void main() {\n"
                "		vec3 newPixel;\n"
                "		vec4 oldPixel = texture2D(u_samplerTexture,texCoord);\n"
				"		newPixel.x = oldPixel.r*0.2126 + oldPixel.g*0.7152+oldPixel.b*0.0722;\n"
				"		newPixel.y =1.0;\n"
				"		newPixel.z = MAX(newPixel.r,newPixel.g,newPixel.b);\n"
                "  	gl_FragColor = vec4(newPixel,1.0);\n"
                "}\n";              
                 */   
             const char gPerspectiveFragmentShader[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision highp float;\n"
                "varying vec2 texCoord;\n"
                "varying float nonlCurve[17];"
                "uniform samplerExternalOES u_samplerTexture;\n"
                "float nonlinearCurveLut(float lumiVal) {\n"
				"lumiVal = lumiVal*255.0;\n"
				"float cumXVal = 0.0;\n"
				"float xDeltN = 256.0;\n"
				"float interpWeight = 0.0;\n"
				"float cumYLeft = 0.0, cumYRight = 0.0;\n"
				"float newLumi = 0.0;\n"
				"for(int i = 0; i < 16; i++) {\n"
				"cumXVal += 256.0;\n"
				"if (lumiVal < cumXVal) {\n"
				"cumYLeft = nonlCurve[i];\n"
				"cumYRight = nonlCurve[i+1];\n"
				"interpWeight = cumXVal - lumiVal;\n"
				"newLumi = interpWeight*cumYLeft + (xDeltN - interpWeight)*cumYRight;\n"
				"newLumi = ((newLumi + 128.0) - (newLumi - (newLumi/256.0)*256.0))/256.0;\n"
				"break;}}\n"
				"return newLumi/255.0;}\n"
                "#define MAX(a,b,c) (a>b?(a>c?a:c):(b>c?b:c))\n"
                "void main() {\n"
                "		vec3 newPixel;\n"
                "		vec4 oldPixel = texture2D(u_samplerTexture,texCoord);\n"
				"		newPixel.x = oldPixel.r*0.2126 + oldPixel.g*0.7152+oldPixel.b*0.0722;\n"
				"		newPixel.y = nonlinearCurveLut(newPixel.x);\n"
				"		newPixel.z = MAX(oldPixel.r,oldPixel.g,oldPixel.b);\n"
                "  	gl_FragColor = vec4(newPixel,1.0);\n"
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

