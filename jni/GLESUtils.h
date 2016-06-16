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
        "precision highp float;\n"
                "varying vec2 texCoord;\n"
                "uniform sampler2D u_samplerTexture;\n"
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
                "void main() {\n"
                "		vec3 newPixel;\n"
                "		vec4 rgbPixel = texture2D(rgbTexture,texCoord);\n"
                "		vec4 maxPixel = texture2D(maxLumTexture,texCoord);\n"
                "		float col = texCoord.x*texSize.z*2.0;\n"
                "		float row = texCoord.y*texSize.w*2.0;\n"
				"		float blkIndexUY = blockCenterIndexUL(row, 255.0, 256.0);\n"
				"		float blkIndexLX = blockCenterIndexUL(col, 255.0, 256.0);\n"
				"		float blkIndexDY = blkIndexUY + 512.0;\n"
				"		float blkIndexRX = blkIndexLX + 512.0;\n"
				"		float blkDWeightY = row - blkIndexUY;\n"
				"		float blkDWeightX = col - blkIndexLX;\n"
				"		float blkDWeightUL = (512.0 - blkDWeightY)*(512.0 - blkDWeightX);\n"
				"		float blkDWeightUR = (512.0 - blkDWeightY)*(blkDWeightX);\n"
				"		float blkDWeightDL = (blkDWeightY)*(512.0 - blkDWeightX);\n"
				"		float blkDWeightDR = (blkDWeightY)*(blkDWeightX);\n"
				"		blkIndexUY = MAX(255.0, blkIndexUY)/512.0;\n"
				"		blkIndexLX = MAX(255.0, blkIndexLX)/512.0;\n"
				"		blkIndexDY = MIN(((texSize.y*512.0) - 255.0), blkIndexDY)/512.0;\n"
				"		blkIndexRX = MIN(((texSize.x*512.0) - 255.0), blkIndexRX)/512.0;\n"
				"		float blkLumiUL = blockLum[int(blkIndexUY*texSize.x + blkIndexLX)];\n"
				"		float blkLumiUR = blockLum[int(blkIndexUY*texSize.x + blkIndexRX)];\n"
				"		float blkLumiDL = blockLum[int(blkIndexDY*texSize.x + blkIndexLX)];\n"
				"		float blkLumiDR = blockLum[int(blkIndexDY*texSize.x + blkIndexRX)];\n"
				"		float blkMapUL = blkMeansGain(blkLumiUL, maxPixel.z*255.0);\n"
				"		float blkMapUR = blkMeansGain(blkLumiUR, maxPixel.z*255.0);\n"
				"		float blkMapDL = blkMeansGain(blkLumiDL, maxPixel.z*255.0);\n"
				"		float blkMapDR = blkMeansGain(blkLumiDR, maxPixel.z*255.0);\n"
				"		float oldGain = ((blkDWeightUL*blkMapUL)/65536.0) + ((blkDWeightUR*blkMapUR)/65536.0) + ((blkDWeightDL*blkMapDL)/65536.0) + ((blkDWeightDR*blkMapDR)/65536.0);\n"
				"		float gain = oldGain;\n"
				//"		if( blkLumiUL > 150 || blkLumiUR > 150 || blkLumiDL > 150 ||blkLumiDR > 150 )\n"
				//"		gain = gain/2;\n"
				"		float channelOR = MIN((rgbPixel.r*255.0*gain + 8192.0)/16384.0, 4095.0)/16.0;\n"
				"		float channelOG = MIN((rgbPixel.g*255.0*gain + 8192.0)/16384.0, 4095.0)/16.0;\n"
				"		float channelOB = MIN((rgbPixel.b*255.0*gain + 8192.0)/16384.0, 4095.0)/16.0;\n"
				"		newPixel = vec3(channelOR/255.0,channelOG/255.0,channelOB/255.0);\n"
                "  	gl_FragColor = vec4(newPixel,1.0);\n"   
                "}\n";
/*
int wdrBase::toneMapping()
{
    LOGD("toneMapping is begin! ");
	int x, y,row,col;
	int channelR, channelG, channelB;
	int channelOR, channelOG, channelOB;
	LONG  oldGain;
	LONG  blkDWeightUL, blkDWeightUR, blkDWeightDL, blkDWeightDR;//64bit
	int rgbOffset = 0, gain;
	int blkRadius, blkCenter;
	int blkIndexUY, blkIndexLX, blkIndexDY, blkIndexRX;
	int blkDWeightX, blkDWeightY;
	int blkLumiUL, blkLumiUR, blkLumiDL, blkLumiDR;
	int blkMapUL, blkMapUR, blkMapDL, blkMapDR;
	//int gainMax = (0xffff>>10)<<10; //sw_wdr_gain_max = 0xffff; gainMax = 64512

	blkRadius = TONE_MAP_BLK_SIZE; //256
	blkCenter = (TONE_MAP_BLK_SIZE>>1)-1 + (TONE_MAP_BLK_SIZE>>1); //xCenter,yCenter = TONE_RADIUS_FIXPOINT_FACTOR( (blkRadius-1 + blkRadius)/2 )

    int offset = 0;
    UINT16 lpLumi;
    UINT16* pMaxLum = mMaxLumiChannel->ptr<UINT16>(0);
    INT32* pBlockLum = mBlockLumiBuff->ptr<INT32>(0);
    UCHAR* pRgb = mSrcImage.ptr<UCHAR>(0);

    for( y = 0; y < mHeight; y++)
    {
        for ( x = 0; x < mWidth; x++)
        {
            //the default Mat's order is BGR
            offset = y * mWidth + x;
        	channelR = *(pRgb+3*offset);
            channelG = *(pRgb+3*offset+1);
            channelB = *(pRgb+3*offset+2);
            lpLumi = *(pMaxLum+offset);

			row = TONE_RADIUS_FIXPOINT_FACTOR(y);   //y*2
			col = TONE_RADIUS_FIXPOINT_FACTOR(x);   //x*2
			// block center coordinates
			blkIndexUY = blockCenterIndexUL(row, blkCenter, blkRadius);
			blkIndexLX = blockCenterIndexUL(col, blkCenter, blkRadius);
			blkIndexDY = blkIndexUY + TONE_RADIUS_FIXPOINT_FACTOR(TONE_MAP_BLK_SIZE);
			blkIndexRX = blkIndexLX + TONE_RADIUS_FIXPOINT_FACTOR(TONE_MAP_BLK_SIZE);

            //find out distance between current pixel and each block;
			blkDWeightY = row - blkIndexUY; //distance from upleft = y(or x) - blockCenter_upleft
			blkDWeightX = col - blkIndexLX;
		    //calculate weight factors of these four blocks (8bit * 8bit)
		    blkDWeightUL = (TONE_WEIGHT_FIXPOINT_FACTOR(1) - blkDWeightY)*(TONE_WEIGHT_FIXPOINT_FACTOR(1) - blkDWeightX);
		    blkDWeightUR = (TONE_WEIGHT_FIXPOINT_FACTOR(1) - blkDWeightY)*(blkDWeightX);
		    blkDWeightDL = (blkDWeightY)*(TONE_WEIGHT_FIXPOINT_FACTOR(1) - blkDWeightX);
	        blkDWeightDR = (blkDWeightY)*(blkDWeightX);

			// block index for finding out corresponding block lumi average
			blkIndexUY = MAX(blkCenter, blkIndexUY) >> TONE_WEIGHT_FIXPOINT_BITS; //boundary clip to get average lumi of each block
			blkIndexLX = MAX(blkCenter, blkIndexLX) >> TONE_WEIGHT_FIXPOINT_BITS;
			blkIndexDY = MIN(((mBlkHeight<<TONE_WEIGHT_FIXPOINT_BITS) - blkCenter), blkIndexDY) >> TONE_WEIGHT_FIXPOINT_BITS;
			blkIndexRX = MIN(((mBlkWidth<<TONE_WEIGHT_FIXPOINT_BITS) - blkCenter), blkIndexRX) >> TONE_WEIGHT_FIXPOINT_BITS;

			//get average luminance of each block( or use data from last frame)
			blkLumiUL = *(pBlockLum + blkIndexUY*mBlkWidth + blkIndexLX);
			blkLumiUR = *(pBlockLum + blkIndexUY*mBlkWidth + blkIndexRX);
			blkLumiDL = *(pBlockLum + blkIndexDY*mBlkWidth + blkIndexLX);
			blkLumiDR = *(pBlockLum + blkIndexDY*mBlkWidth + blkIndexRX);

			blkMapUL = blkMeansGain(blkLumiUL, lpLumi);
			blkMapUR = blkMeansGain(blkLumiUR, lpLumi);
			blkMapDL = blkMeansGain(blkLumiDL, lpLumi);
			blkMapDR = blkMeansGain(blkLumiDR, lpLumi);

            oldGain = ((blkDWeightUL*blkMapUL)>>16) + ((blkDWeightUR*blkMapUR)>>16) +
                    ((blkDWeightDL*blkMapDL)>>16) + ((blkDWeightDR*blkMapDR)>>16);
			gain = (int)oldGain;
			//if(channelR)
			//gain = MIN(gain, gainMax);
            //get current pixel's luminance after tone mapping
			channelOR = MIN(FIXPOINT_REVERT((channelR+rgbOffset)*gain
			        , TONE_GAIN_FIXPOINT_BITS), BIT_MASK(12));
            channelOG = MIN(FIXPOINT_REVERT((channelG+rgbOffset)*gain
                    , TONE_GAIN_FIXPOINT_BITS), BIT_MASK(12));
            channelOB = MIN(FIXPOINT_REVERT((channelB+rgbOffset)*gain
                    , TONE_GAIN_FIXPOINT_BITS), BIT_MASK(12));
            channelOR = (channelOR>>4);
            channelOG = (channelOG>>4);
            channelOB = (channelOB>>4);
            *(pRgb+3*offset) = (UINT8)channelOR;
            *(pRgb+3*offset+1) = (UINT8)channelOG;
            *(pRgb+3*offset+2) = (UINT8)channelOB;

        }
    }
    LOGD("toneMapping is end! ");

    return 0;
} */


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

