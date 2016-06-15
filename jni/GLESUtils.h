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
const char gStep1VertexShader[] =
        "attribute vec4 a_position;\n"
                "uniform vec2 textureSize;\n"
                "attribute vec2 a_texCoord;\n"
                "varying vec2 texCoord;\n"
                "void main() {\n"
                "  texCoord = a_texCoord;\n"
                "  gl_Position = a_position;\n"
                "}\n";
const char gStep1FragmentShader[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision highp float;\n"
                "varying vec2 texCoord;\n"
                "uniform float nonlCurve[17];"
                "uniform samplerExternalOES u_samplerTexture;\n"
                "float nonlinearCurveLut(float lumiVal) {\n"
				"lumiVal = lumiVal*255.0;\n"
				"float cumXVal = 0.0;\n"
				"float xDeltN = 255.0;\n"
				"float interpWeight = 0.0;\n"
				"float cumYLeft = 0.0, cumYRight = 0.0;\n"
				"float newLumi = 0.0;\n"
				"for(int i = 0; i < 16; i++) {\n"
				"cumXVal += 255.0;\n"
				"if (lumiVal < cumXVal) {\n"
				"cumYLeft = nonlCurve[i];\n"
				"cumYRight = nonlCurve[i+1];\n"
				"interpWeight = cumXVal - lumiVal;\n"
				"newLumi = interpWeight*cumYLeft + (xDeltN - interpWeight)*cumYRight;\n"
				"newLumi = (newLumi + 128.0)/255.0;\n"
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

const char gStep2VertexShader[] =
        "attribute vec4 a_position;\n"
                "uniform vec2 textureSize;\n"
                "attribute vec2 a_texCoord;\n"
                "varying vec2 texCoord;\n"
                "void main() {\n"
                "  texCoord = a_texCoord;\n"
                "  gl_Position = a_position;\n"
                "}\n";

const char gStep2FragmentShader[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision highp float;\n"
                "varying vec2 texCoord;\n"
                "uniform samplerExternalOES u_samplerTexture;\n"
                "void main() {\n"
                "		vec4 oldPixel = texture2D(u_samplerTexture,texCoord);\n"
                " 		gl_FragColor = vec4( 0.0,oldPixel.y,0.0,1.0);\n"
                "}\n";


const char gToneMapVertexShader[] =
        "attribute vec4 a_position;\n"
                "uniform vec4 textureSize;\n"
                "attribute vec2 a_texCoord;\n"
                "varying vec2 texCoord;\n"
                "varying vec4 texSize;\n"
                "void main() {\n"
                "   texSize = textureSize;\n"
                "  texCoord = a_texCoord;\n"
                "  gl_Position = a_position;\n"
                "}\n";
                
const char gToneMapFragmentShader[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision highp float;\n"
                "varying vec2 texCoord;\n"
                "varying vec4 texSize;\n"
                "uniform float blockLum[256];\n"      			  //x,y 存放block 大小，z,w存放rgb纹理大小
                "uniform sampler2D maxLumTexture;\n"
                "uniform samplerExternalOES rgbTexture;\n"
                "#define MIN(a,b) ((a) <= (b) ? (a):(b))\n"
				"#define MAX(a,b) ((a) >= (b) ? (a):(b))\n"
                "float blkMeansGain(float blkLumi, float lpLumi) {\n"
				"float lumiGain, newLumi;\n"
				"newLumi =  4095.0 + (blkLumi*lpLumi /4095.0);\n"
				"lumiGain = (16383.0*newLumi)/(blkLumi + lpLumi + 410.0);\n"
				"lumiGain = MIN(lumiGain, 262143.0);\n"
				"return lumiGain;}\n"
				"float blockCenterIndexUL(float x, float blkCenter, float blkRadius) {\n"
				"float blkSizeOffset;\n"
				"float blkIndex;\n"
				"blkSizeOffset = x - blkCenter;\n"
				"if (blkSizeOffset < 0.0) {blkIndex = -(blkRadius+1.0);}\n"
				"else{blkSizeOffset = blkSizeOffset/511.0;\n"
				"blkIndex = (blkSizeOffset*511.0) + blkCenter;}\n"
				"return blkIndex;}\n"
				"float gain = 0.0,oldGain =  0.0;\n"
				"float channelOR = 0.0,channelOG =  0.0,channelOB =  0.0;\n"
				"float blkIndexUY = 0.0,blkIndexLX =  0.0,blkIndexDY = 0.0,blkIndexRX =  0.0;\n"
				"float blkDWeightUL = 0.0,blkDWeightUR =  0.0,blkDWeightDL = 0.0,blkDWeightDR =  0.0;\n"
				"float blkMapUL = 0.0,blkMapUR =  0.0,blkMapDL = 0.0,blkMapDR =  0.0;\n" 
                "void main() {\n"
                "		vec3 newPixel;\n"
                "		vec4 rgbPixel = texture2D(rgbTexture,texCoord);\n"
                "		vec4 maxPixel = texture2D(maxLumTexture,texCoord);\n"
                "		float col = texCoord.x*texSize.z*2.0;\n"
                "		float row = texCoord.y*texSize.w*2.0;\n"
				"		blkIndexUY = blockCenterIndexUL(row, 255.0, 256.0);\n"
				"		blkIndexLX = blockCenterIndexUL(col, 255.0, 256.0);\n"
				"		blkIndexDY = blkIndexUY + 512.0;\n"
				"		blkIndexRX = blkIndexLX + 512.0;\n"
				"		float blkDWeightY = row - blkIndexUY;\n"
				"		float blkDWeightX = col - blkIndexLX;\n"
				"		blkDWeightUL = (511.0 - blkDWeightY)*(511.0 - blkDWeightX);\n"
				"		blkDWeightUR = (511.0 - blkDWeightY)*(blkDWeightX);\n"
				"		blkDWeightDL = (blkDWeightY)*(511.0 - blkDWeightX);\n"
				"		blkDWeightDR = (blkDWeightY)*(blkDWeightX);\n"
				"		blkIndexUY = MAX(255.0, blkIndexUY)/511.0;\n"
				"		blkIndexLX = MAX(255.0, blkIndexLX)/511.0;\n"
				"		blkIndexDY = MIN(((texSize.y*511.0) - 255.0), blkIndexDY)/511.0;\n"
				"		blkIndexRX = MIN(((texSize.x*511.0) - 255.0), blkIndexRX)/511.0;\n"
				"		float blkLumiUL = blockLum[int(blkIndexUY*texSize.x + blkIndexLX)];\n"
				"		float blkLumiUR = blockLum[int(blkIndexUY*texSize.x + blkIndexRX)];\n"
				"		float blkLumiDL = blockLum[int(blkIndexDY*texSize.x + blkIndexLX)];\n"
				"		float blkLumiDR = blockLum[int(blkIndexDY*texSize.x + blkIndexRX)];\n"
				"		blkMapUL = blkMeansGain(blkLumiUL, maxPixel.z);\n"
				"		blkMapUR = blkMeansGain(blkLumiUR, maxPixel.z);\n"
				"		blkMapDL = blkMeansGain(blkLumiDL, maxPixel.z);\n"
				"		blkMapDR = blkMeansGain(blkLumiDR, maxPixel.z);\n"
				"		oldGain = ((blkDWeightUL*blkMapUL)/65535.0) + ((blkDWeightUR*blkMapUR)/65535.0) + ((blkDWeightDL*blkMapDL)/65535.0) + ((blkDWeightDR*blkMapDR)/65535.0);\n"
				"		gain = oldGain;\n"
				"		channelOR = MIN((rgbPixel.r*255.0*gain + 8191.0)/16383.0, 4094.0)/16.0;\n"
				"		channelOG = MIN((rgbPixel.g*255.0*gain + 8191.0)/16383.0, 4094.0)/16.0;\n"
				"		channelOB = MIN((rgbPixel.b*255.0*gain + 8191.0)/16383.0, 4094.0)/16.0;\n"
				"		newPixel = vec3(channelOR/255.0,channelOG/255.0,channelOB/255.0);\n"
                "  	gl_FragColor = vec4(newPixel,1.0);\n"   
                "}\n"; 


const GLfloat vVertices[] = {
		// X,  Y, Z, W,    U, V
		1,	1, 0, 1,	1, 1, //Top Right
		-1,  1, 0, 1,	 0, 1, //Top Left
		-1, -1, 0, 1,	 0, 0, // Bottom Left
		1, -1, 0, 1,	1, 0 //Bottom Right
};

const GLushort indices[] = { 0, 3, 2, 2, 1, 0 };

class GLESUtils
{
public:
    GLESUtils(int buf_cnt);
    ~GLESUtils();

public:
    int initOpenGLES(alloc_device_t *m_alloc_dev, int width, int height);
    int Progress(int* targetAddr, int mode);
    bool setMode(int);
	int updateImageData(struct cv_fimc_buffer *m_buffers_capture);
    int GLProcess(int vpWidth, int vpHeight, int fboId, int* targetAddr, int mode);
	int getResult(int targetAddr);
	unsigned long getTickCount() ;
private:
	int mCurrentId;
	int mImageCnt;
    double work_begin;
    double work_end;
    float mWidth;
    float mHeight;
    int mBlkWidth;
    int mBlkHeight;
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

	GLuint programStep1Object;
	GLuint programStep2Object;
	GLuint mStep1FboId;
	GLuint mStep2FboId;
	GLuint mStep1TexId;
	GLuint mStep2TexId;
	EGLImageKHR mStep1EGLImage;
	EGLImageKHR mStep2EGLImage;
	sp <GraphicBuffer> mStep1GraphicBuffer;
	sp <GraphicBuffer> mStep2GraphicBuffer;

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
	int loadProgramUniforms(GLuint program,GLuint stepIndex);
    void initializeTmpResEGLImage(alloc_device_t *m_alloc_dev, int fboWidth, int fboHeight, GLuint *tex,
                                  GLuint * fbo, GLuint texGroup);
	void createFBO(int width, int height, GLuint* fbo);
	void createFBOTexture1(alloc_device_t *m_alloc_dev, int fboWidth, int fboHeight, GLuint *tex,
                                        GLuint * fbo, GLuint texGroup);
	void createFBOTexture2(alloc_device_t *m_alloc_dev, int fboWidth, int fboHeight, GLuint *tex,
                                        GLuint * fbo, GLuint texGroup);
};
#endif //GLESUTILS_H

