//
// Created by linqi on 16-4-5.
//
#include "GLESUtils.h"
#include "opencv2/opencv.hpp"
#include <gui/GLConsumer.h>
#include <gui/Surface.h>
#include <gui/IGraphicBufferConsumer.h>
#include <gui/IGraphicBufferProducer.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <gralloc_priv.h>
#include "utils.h"
#include "stdio.h"
#include <time.h>
#include <stdlib.h>
using namespace std;
using namespace cv;
using namespace android;

GLint TEXTURE_GROUP[] = {
	GL_TEXTURE0,
	GL_TEXTURE1,
	GL_TEXTURE2,
	GL_TEXTURE3,
	GL_TEXTURE4,
	GL_TEXTURE5,
	GL_TEXTURE6
};

void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGD("after %s() glError (0x%x)\n", op, error);
    }
}

GLESUtils::GLESUtils(int buf_cnt)
{
    work_begin = 0;
    work_end = 0;
    mBlkWidth = 0;
    mBlkHeight = 0;
    mWidth = 0;
    mHeight = 0;
    checkInitOpenGLES = false;
	mCurrentId = 0;
	mImageCnt = buf_cnt;
	
	for (int i=0; i<mImageCnt; i++) {
		mCameraGLTexImage[i] = NULL;
	}
}

GLESUtils::~GLESUtils()
{
    for (int i=0; i<mImageCnt; i++)
    {
		mCameraGLTexImage[i]->clear();
    }

	mStep1GraphicBuffer = NULL;
    eglDestroyImageKHR(display, mStep1EGLImage);
    mStep1EGLImage = EGL_NO_IMAGE_KHR;
	
    mStep2GraphicBuffer = NULL;
    eglDestroyImageKHR(display, mStep2EGLImage);
    mStep2EGLImage = EGL_NO_IMAGE_KHR;

    glUseProgram(0);
    glDeleteProgram(programObject);
    glDeleteTextures(1,&textureID1);
    glDeleteTextures(1,&textureID2);
    glDeleteTextures(1,&textureID3);
    glDeleteTextures(1,&textureID4);
    glDeleteTextures(1,&textureID5);
    glDeleteTextures(1,&textureID6);
    DestroyEGL();
}

bool GLESUtils::setMode(int homoMethod )
{
    return true;
}

int GLESUtils::loadProgramUniforms(GLuint program,GLuint stepIndex) {
		GLuint positionHandle;
		GLuint texCoordHandle;
		GLsizei stride = 6*sizeof(GLfloat);
		glUseProgram(program);
		//get attribution unit index
		positionHandle = glGetAttribLocation(program, "a_position" );
		texCoordHandle = glGetAttribLocation(program, "a_texCoord" );
		// Load the vertex position
		glVertexAttribPointer(positionHandle, 4, GL_FLOAT, GL_FALSE, stride, vVertices);
		// Load the texture coordinate
		glVertexAttribPointer(texCoordHandle, 2, GL_FLOAT, GL_FALSE, stride, &vVertices[4]);
	    glEnableVertexAttribArray(positionHandle);
		glEnableVertexAttribArray(texCoordHandle);
		
	if(stepIndex == 1) {
		vSizeHandle = glGetUniformLocation(program, "textureSize");
		float gSize[2] = {mWidth,mHeight};
		glUniform2fv(vSizeHandle,1,gSize);
		checkGlError("loadProgramUniforms-glGetUniformLocation");
		GLfloat nonlCurve[17] = {0.0,887.0,1360.0,1714.0,2007.0,2261.0,2489.0,2697.0,2889.0,3069.0,3237.0,3397.0,3549.0,3694.0,3833.0,3967.0,4096.0};
		glUniform1fv(glGetUniformLocation(program, "nonlCurve"), 17, nonlCurve);
		checkGlError("loadProgramUniforms-glGetUniformLocation:nonlCurve");
		
	} else if(stepIndex == 2) {
		vSizeHandle = glGetUniformLocation(program, "textureSize");
		float gSize[2] = {mWidth,mHeight};
		glUniform2fv(vSizeHandle,1,gSize);
		checkGlError("loadProgramUniforms-glGetUniformLocation");
		
	}else if(stepIndex == 3) {
		vSizeHandle = glGetUniformLocation(program, "textureSize");
		float gSize[4] = {mBlkWidth,mBlkHeight,mWidth,mHeight};
		glUniform4fv(vSizeHandle,1,gSize);
		checkGlError("loadProgramUniforms-glGetUniformLocation");
	}else {
		LOGE("loadProgramUniforms stepIndex is out of range(1,2,3 is the right number)");
		return GL_FALSE;
	}
	 return GL_TRUE;
}

int GLESUtils::initOpenGLES(alloc_device_t *m_alloc_dev, int width, int height)
{
    checkInitOpenGLES = true;

    mWidth = width;
    mHeight = height;
	mBlkWidth = (int)mWidth >> 8;
	mBlkHeight = (int)mHeight >> 8;
	if ((int)mWidth - (mBlkWidth << 8) > 0)
		mBlkWidth += 1;
	if ((int)mHeight - (mBlkHeight << 8) > 0)
		mBlkHeight += 1;
    // Init EGL display, surface and context
    if(!InitEGL())
    {
        LOGE("Init EGL fail\n");
        return GL_FALSE;
    } 
	//gToneMapVertexShader   gToneMapFragmentShader   gPerspectiveVertexShader    gPerspectiveFragmentShader
    programObject = LoadProgram(gToneMapVertexShader,gToneMapFragmentShader);
	programStep1Object = LoadProgram(gStep1VertexShader,gStep1FragmentShader);
	programStep2Object = LoadProgram(gStep2VertexShader,gStep2FragmentShader);
	LOGD("load program: %d, %d, %d", programObject, programStep1Object, programStep2Object);
    checkGlError("LoadProgram");

	loadProgramUniforms(programStep1Object,1);
	loadProgramUniforms(programStep2Object,2);
	loadProgramUniforms(programObject,3);

	//createFBO(160, 90, &renderFboId);
	//GLuint tex;
	//createFBOTexture(m_alloc_dev, 160, 90, &tex, &renderFboId, GL_TEXTURE8);
	createFBOTexture1(m_alloc_dev, (int) mWidth, (int) mHeight, &mStep1TexId, &mStep1FboId, GL_TEXTURE7);
	createFBOTexture2(m_alloc_dev, mBlkWidth, mBlkHeight, &mStep2TexId, &mStep2FboId, GL_TEXTURE8);
    initializeTmpResEGLImage(m_alloc_dev, (int) mWidth, (int) mHeight, &targetTexId, &fboTargetHandle, GL_TEXTURE9);
    checkGlError("initializeTmpResEGLImage");

	LOGD("init egl env done.");

    return GL_TRUE;
}

void GLESUtils::createFBO(int fboWidth, int fboHeight, GLuint* fbo) {
	glGenFramebuffers(1, fbo);
    LOGD("generate rbo/fbo for target fbo id: %d, w-h: %d-%d", *fbo, fboWidth, fboHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
	checkGlError("createFBO-glBindFramebuffer");

	GLuint rboId;
	glGenRenderbuffers(1, &rboId);
	glBindRenderbuffer(GL_RENDERBUFFER, rboId);
	checkGlError("createFBO-glBindRenderbuffer");
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, fboWidth , fboHeight);
    checkGlError("createFBO-glRenderbufferStorage");

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboId);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGD("framebuffer status check fail: %d", status);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLESUtils::createFBOTexture1(alloc_device_t *m_alloc_dev, int fboWidth, int fboHeight, GLuint *tex,
                                        GLuint * fbo, GLuint texGroup) {
    glGenTextures(1, tex);
    glActiveTexture(texGroup);
    checkGlError("initializeTmpResEGLImage-gentex");
    glBindTexture(GL_TEXTURE_2D, *tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError("initializeTmpResEGLImage-inittex");
	
	if (m_alloc_dev == NULL) {
	    mStep1GraphicBuffer = new GraphicBuffer(fboWidth, fboHeight, HAL_PIXEL_FORMAT_RGBA_8888,
	                                             GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_SW_WRITE_RARELY);
	} else {
	 	int stride;
		int err = m_alloc_dev->alloc(m_alloc_dev,
	                                         fboWidth,
	                                         fboHeight,
	                                         HAL_PIXEL_FORMAT_RGBA_8888,
	                                         GraphicBuffer::USAGE_HW_TEXTURE,
	                                         &mTargetBufferHandle,
	                                         &stride);

		mStep1GraphicBuffer = new GraphicBuffer(fboWidth, fboHeight, HAL_PIXEL_FORMAT_RGBA_8888,
	                GraphicBuffer::USAGE_HW_TEXTURE, fboWidth, (native_handle_t*)mTargetBufferHandle, false);

	}
	
    EGLClientBuffer clientBuffer = (EGLClientBuffer)mStep1GraphicBuffer->getNativeBuffer();
    mStep1EGLImage = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                                        clientBuffer, 0);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)mStep1EGLImage);
    checkGlError("initializeTmpResEGLImage-glEGLImageTargetTexture2DOES");

	glGenFramebuffers(1, fbo);
    LOGD("generate tex/fbo for target tex id: %d, fbo id: %d, w-h: %d-%d", *tex, *fbo, fboWidth, fboHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    checkGlError("initializeTmpResEGLImage-glFramebufferTexture2D");

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGD("framebuffer statuc check fail: %d", status);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
void GLESUtils::createFBOTexture2(alloc_device_t *m_alloc_dev, int fboWidth, int fboHeight, GLuint *tex,
                                        GLuint * fbo, GLuint texGroup) {
    glGenTextures(1, tex);
    glActiveTexture(texGroup);
    checkGlError("initializeTmpResEGLImage-gentex");
    glBindTexture(GL_TEXTURE_2D, *tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError("initializeTmpResEGLImage-inittex");
	
	if (m_alloc_dev == NULL) {
	    mStep2GraphicBuffer = new GraphicBuffer(fboWidth, fboHeight, HAL_PIXEL_FORMAT_RGBA_8888,
	                                             GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_SW_WRITE_RARELY);
	} else {
	 	int stride;
		int err = m_alloc_dev->alloc(m_alloc_dev,
	                                         fboWidth,
	                                         fboHeight,
	                                         HAL_PIXEL_FORMAT_RGBA_8888,
	                                         GraphicBuffer::USAGE_HW_TEXTURE,
	                                         &mTargetBufferHandle,
	                                         &stride);

		mStep2GraphicBuffer = new GraphicBuffer(fboWidth, fboHeight, HAL_PIXEL_FORMAT_RGBA_8888,
	                GraphicBuffer::USAGE_HW_TEXTURE, fboWidth, (native_handle_t*)mTargetBufferHandle, false);

	}
	
    EGLClientBuffer clientBuffer = (EGLClientBuffer)mStep2GraphicBuffer->getNativeBuffer();
    mStep2EGLImage = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                                        clientBuffer, 0);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)mStep2EGLImage);
    checkGlError("initializeTmpResEGLImage-glEGLImageTargetTexture2DOES");

	glGenFramebuffers(1, fbo);
    LOGD("generate tex/fbo for target tex id: %d, fbo id: %d, w-h: %d-%d", *tex, *fbo, fboWidth, fboHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    checkGlError("initializeTmpResEGLImage-glFramebufferTexture2D");

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGD("framebuffer statuc check fail: %d", status);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void GLESUtils::initializeTmpResEGLImage(alloc_device_t *m_alloc_dev, int fboWidth, int fboHeight, GLuint *tex,
                                        GLuint * fbo, GLuint texGroup)
{
    glGenTextures(1, tex);
    glActiveTexture(texGroup);
    checkGlError("initializeTmpResEGLImage-gentex");
    glBindTexture(GL_TEXTURE_2D, *tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError("initializeTmpResEGLImage-inittex");

	if (m_alloc_dev == NULL) {
	    mTargetGraphicBuffer = new GraphicBuffer(fboWidth, fboHeight, HAL_PIXEL_FORMAT_RGBA_8888,
	                                             GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_SW_WRITE_RARELY);
	} else {
	 	int stride;
		int err = m_alloc_dev->alloc(m_alloc_dev,
	                                         fboWidth,
	                                         fboHeight,
	                                         HAL_PIXEL_FORMAT_RGBA_8888,
	                                         GraphicBuffer::USAGE_HW_TEXTURE,
	                                         &mTargetBufferHandle,
	                                         &stride);

		mTargetGraphicBuffer = new GraphicBuffer(fboWidth, fboHeight, HAL_PIXEL_FORMAT_RGBA_8888,
	                GraphicBuffer::USAGE_HW_TEXTURE, fboWidth, (native_handle_t*)mTargetBufferHandle, false);

	}
	
    EGLClientBuffer clientBuffer = (EGLClientBuffer)mTargetGraphicBuffer->getNativeBuffer();
    mTargetEGLImage = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                                        clientBuffer, 0);

    //glBindTexture(GL_TEXTURE_EXTERNAL_OES, _textureid);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)mTargetEGLImage);
    checkGlError("initializeTmpResEGLImage-glEGLImageTargetTexture2DOES");

    glGenFramebuffers(1, fbo);
    LOGD("generate tex/fbo for target tex id: %d, fbo id: %d, w-h: %d-%d", *tex, *fbo, fboWidth, fboHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    checkGlError("initializeTmpResEGLImage-glFramebufferTexture2D");

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGD("framebuffer statuc check fail: %d", status);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int GLESUtils::updateImageData(struct cv_fimc_buffer *m_buffers_capture) {
	//workBegin();
    {
    	//LOGD("(%d) fill grey mat vector, fd: %d, w-h = %d-%d, start: %.8x, length: %d", gettid(), m_buffers_capture->share_fd, (int)mWidth, (int)mHeight, m_buffers_capture->start, m_buffers_capture->length);

		for (int i=0; i<mImageCnt; i++) {
			if (mCameraGLTexImage[i] == NULL) {
				mCurrentId = i;
				break;
			}

			if (mCameraGLTexImage[i]->mShareFd > 0 && mCameraGLTexImage[i]->mShareFd == m_buffers_capture->share_fd) {
				mCurrentId = i;
				break;
			}
		}

		if (mCameraGLTexImage[mCurrentId] == NULL) {
			//LOGD("create image id: %d", mCurrentId);
		    mCameraGLTexImage[mCurrentId] = new CameraGLTexImage(mCurrentId, display, (char*)m_buffers_capture->start, (m_buffers_capture->handle), m_buffers_capture->share_fd);
		    mCameraGLTexImage[mCurrentId]->createTexImage(mWidth, mHeight, m_buffers_capture->format);//HAL_PIXEL_FORMAT_YV12);//HAL_PIXEL_FORMAT_YCrCb_NV12);
		}
		checkGlError("createTexImage");
		//LOGD("create tex image done");
	    if (m_buffers_capture->handle == NULL && m_buffers_capture->share_fd == 0) {
			//LOGD("update image id: %d", mCurrentId);
	    	mCameraGLTexImage[mCurrentId]->updateTexImage(mWidth, mHeight, m_buffers_capture->format);//HAL_PIXEL_FORMAT_YCrCb_NV12);
	    }
		checkGlError("updateTexImage");
		//LOGD("update tex image done");

	}
	checkGlError("updateImageData");
	//workEnd("update-image");
}

int GLESUtils::Progress(int* targetAddr, int mode)
{
    if(checkInitOpenGLES == false)
    {
        assert("Please initOpenGLES first!");
        return 0;
    }
	
    workBegin();
    int location;
	
	glUseProgram(programStep1Object);
    // Set the sampler texture unit index
    location = glGetUniformLocation(programStep1Object, "u_samplerTexture");
    glUniform1i(location, mCurrentId);
    //LOGD("glGetUniformLocation u_samplerTexture: %d, id: %d", location, mCurrentId);
    checkGlError("GLProcess-glUniform1i");
	GLProcess(mWidth, mHeight, mStep1FboId, NULL, OUTPUT_NONE);
	
	glUseProgram(programStep2Object);
    // Set the sampler texture unit index
    location = glGetUniformLocation(programStep2Object, "u_samplerTexture");
    glUniform1i(location, 7);
    //LOGD("glGetUniformLocation u_samplerTexture: %d, id: %d", location, 7);
    checkGlError("GLProcess-glUniform1i");
	GLProcess(mWidth, mHeight, mStep2FboId, NULL, OUTPUT_NONE);
	
	// Set the sampler texture unit index
	glUseProgram(programObject);
    int location1 = glGetUniformLocation(programObject, "maxLumTexture");
    glUniform1i(location1, 7);
    //LOGD("glGetUniformLocation u_samplerTexture: %d, id: %d", location, 7);
    checkGlError("GLProcess-glUniform1i");
    int location2 = glGetUniformLocation(programObject, "rgbTexture");
    glUniform1i(location2, mCurrentId);
    //LOGD("glGetUniformLocation u_samplerTexture: %d, id: %d", location2, mCurrentId);
    checkGlError("GLProcess-glUniform1i");
    char* blockBufferAddr = NULL;
    int err = mStep2GraphicBuffer->lock(GRALLOC_USAGE_SW_READ_RARELY, (void **) (&blockBufferAddr));
    if (err != 0 || blockBufferAddr == NULL) {
	    LOGD("mYUVTexBuffer->lock(...) failed: %d\n", err);
	    return -1;
	}
	Mat block(mBlkHeight, mBlkWidth, CV_8UC4, (void*)blockBufferAddr);
	vector<Mat> blockChannel;
	split(block,blockChannel);
	unsigned char *blockPtr = NULL;
	GLfloat blockBuff[256];
	blockPtr = (unsigned char *)blockChannel[1].data;
	for(int i = 0; i < mBlkHeight*mBlkWidth;i++) {
		//第二个通道存放的才是有用的数据
		blockBuff[i] = *blockPtr++;
	}
    err = mStep2GraphicBuffer->unlock();
	if (err != 0) {
		LOGD("mYUVTexBuffer->unlock() failed: %d\n", err);
	    return -1;
	}
	glUniform1fv(glGetUniformLocation(programObject, "blockLum"), 256, blockBuff);
    GLProcess(mWidth, mHeight, fboTargetHandle, targetAddr, mode);
    //LOGD("glGetUniformLocation u_samplerTexture: %d, id: %d", location2, mCurrentId);
    workEnd("GLProcess");
    return 1;
}

//render in here
int GLESUtils::GLProcess(int vpWidth, int vpHeight, int fboId, int* targetAddr, int mode)
{
	LOGD("%s(%d)-<%s>, tid(%d)",__FILE__, __LINE__, __FUNCTION__, gettid());

    //begin to render
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    glViewport(0, 0, vpWidth, vpHeight);
	
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
    glFinish();

    //workEnd();

	checkGlError("GLProcess draw");
	{
		if (mode == OUTPUT_NONE) {
			// do nothing
			return 0;
		}
	    int err = mTargetGraphicBuffer->lock(GRALLOC_USAGE_SW_READ_RARELY, (void **) (&mTargetGraphicBufferAddr));
	    if (err != 0 || mTargetGraphicBufferAddr == NULL)
	    {
	        LOGD("mYUVTexBuffer->lock(...) failed: %d\n", err);
	        return -1;
	    }

		if (mode == OUTPUT_FMT_CV_MAT) {

		}

		if (mode == OUTPUT_FMT_YUV) {
			//RGBA2YUV420SP(mWidth, mHeight, (char*)mTargetGraphicBufferAddr, (char*)targetAddr);
			rga_copy_and_scale(mWidth, mHeight, (int)mTargetGraphicBufferAddr, RK_FORMAT_RGBA_8888,
												mWidth, mHeight, (int)targetAddr, RK_FORMAT_YCbCr_420_SP);//RK_FORMAT_YCrCb_420_SP);
		}

		if (mode == OUTPUT_FMT_RGBA) {
			/*
			//void* addr = malloc(160*90*4);
			//glReadPixels(0, 0, 160, 90, GL_RGBA, GL_UNSIGNED_BYTE, addr);
			//targetAddr = (int)mTargetGraphicBufferAddr;
			FILE* fp = fopen("/data/local/result.data", "ab+");
			if(fp == NULL)  
		    {  
		        LOGD("open /data/local/result.data failed\n");
		    } else {
		    	LOGD("write /data/local/result.data\n");
				fwrite(mTargetGraphicBufferAddr, 1, vpWidth * vpHeight * 4, fp);
				fclose(fp);
		    }
			*/
			*targetAddr = (int)mTargetGraphicBufferAddr;

			//memcpy(targetAddr, mTargetGraphicBufferAddr, mHeight * mWidth * 4);
		}

	    err = mTargetGraphicBuffer->unlock();
	    if (err != 0)
	    {
	        LOGD("mYUVTexBuffer->unlock() failed: %d\n", err);
	        return -1;
	    }
	}
    //workEnd("COMPOSITION");

    return GL_TRUE;
}

int GLESUtils::getResult(int targetAddr) {
	int err = mTargetGraphicBuffer->lock(GRALLOC_USAGE_SW_READ_RARELY, (void **) (&mTargetGraphicBufferAddr));
	if (err != 0 || mTargetGraphicBufferAddr == NULL)
	{
		LOGD("mYUVTexBuffer->lock(...) failed: %d\n", err);
		return -1;
	}

	rga_copy_and_scale(mWidth, mHeight, (int)mTargetGraphicBufferAddr, RK_FORMAT_RGBA_8888,
													mWidth, mHeight, targetAddr, RK_FORMAT_YCbCr_420_SP);//RK_FORMAT_YCrCb_420_SP);
	
	err = mTargetGraphicBuffer->unlock();
	if (err != 0)
	{
		LOGD("mYUVTexBuffer->unlock() failed: %d\n", err);
		return -1;
	}
}

int GLESUtils::DestroyEGL()
{
    //Typical egl cleanup
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    //eglDestroySurface(display, mSurface);
    eglDestroyContext(display, context);
    eglTerminate(display);
    eglReleaseThread();

	return 0;
}

int GLESUtils::InitEGL()
{

    NativeWindowType eglWindow = NULL;
    surface = NULL;

    EGLConfig configs[2];
    EGLBoolean eRetStatus;
    EGLint majorVer, minorVer;
    EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    EGLint numConfigs;
    EGLint cfg_attribs[] = {EGL_BUFFER_SIZE,    EGL_DONT_CARE,
                            EGL_DEPTH_SIZE,     16,
                            EGL_RED_SIZE,       8,
                            EGL_GREEN_SIZE,     8,
                            EGL_BLUE_SIZE,      8,
                            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                            EGL_NONE};

    // Get default display connection
    display = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
    if ( display == EGL_NO_DISPLAY )
    {
        return EGL_FALSE;
    }

    // Initialize EGL display connection
    eRetStatus = eglInitialize(display, &majorVer, &minorVer);
    if( eRetStatus != EGL_TRUE )
    {
        return EGL_FALSE;
    }

    //Get a list of all EGL frame buffer configurations for a display
    eRetStatus = eglGetConfigs (display, configs, 2, &numConfigs);
    if( eRetStatus != EGL_TRUE )
    {
        return EGL_FALSE;
    }

    // Get a list of EGL frame buffer configurations that match specified attributes
    eRetStatus = eglChooseConfig (display, cfg_attribs, configs, 2, &numConfigs);
    if( eRetStatus != EGL_TRUE  || !numConfigs)
    {
        return EGL_FALSE;
    }

    sp<IGraphicBufferProducer> producer;
    sp<IGraphicBufferConsumer> consumer;
    BufferQueue::createBufferQueue(&producer, &consumer);
    sp<GLConsumer> mST = new GLConsumer(consumer, 123, GLConsumer::TEXTURE_EXTERNAL, true,
            false);
    mST->setDefaultBufferSize(64, 64);
    mST->setDefaultBufferFormat(HAL_PIXEL_FORMAT_RGBA_8888);
    sp<Surface> mSTC = new Surface(producer);
    sp<ANativeWindow> window = mSTC.get();
    eglWindow = window.get();

    //LOGE("eglCreateWindowSurface");
    // Create a new EGL window surface
    surface = eglCreateWindowSurface(display, configs[0], eglWindow, NULL);
    //if (surface == EGL_NO_SURFACE)
    //{
    //return EGL_FALSE;
    //}


    // Set the current rendering API (EGL_OPENGL_API, EGL_OPENGL_ES_API,EGL_OPENVG_API)
    eRetStatus = eglBindAPI(EGL_OPENGL_ES_API);
    if (eRetStatus != EGL_TRUE)
    {
        return EGL_FALSE;
    }

    // Create a new EGL rendering context
    context = eglCreateContext (display, configs[0], EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT)
    {
        return EGL_FALSE;
    }

    // Attach an EGL rendering context to EGL surfaces
    eRetStatus = eglMakeCurrent (display, surface, surface, context);
    if( eRetStatus != EGL_TRUE )
    {
        return EGL_FALSE;
    }
    //If interval is set to a value of 0, buffer swaps are not synchronized to a video frame, and the swap happens as soon as the render is complete.
    eglSwapInterval(display,0);


    return EGL_TRUE;
}

GLuint GLESUtils::LoadShader( GLenum type, const char *shaderSrc )
{
    GLuint shader;
    GLint compiled;

    // Create an empty shader object, which maintain the source code strings that define a shader
    shader = glCreateShader ( type );

    if ( shader == 0 )
        return 0;

    // Replaces the source code in a shader object
    glShaderSource ( shader, 1, &shaderSrc, NULL );
    // Compile the shader object
    glCompileShader ( shader );

    // Check the shader object compile status
    glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

    if ( !compiled )
    {
        GLint infoLen = 0;
        glGetShaderiv ( shader, GL_INFO_LOG_LENGTH, &infoLen );

        if ( infoLen > 1 )
        {
            char* infoLog = (char*)malloc (sizeof(char) * infoLen );
            glGetShaderInfoLog ( shader, infoLen, NULL, infoLog );
            LOGE ( "Error compiling shader:\n%s\n", infoLog );
            free ( infoLog );
        }

        glDeleteShader ( shader );
        return 0;
    }

    return shader;
}

GLuint GLESUtils::LoadProgram( const char *vShaderStr, const char *fShaderStr)
{
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;

    // Load the vertex/fragment shaders
    vertexShader = LoadShader ( GL_VERTEX_SHADER, vShaderStr );
    fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fShaderStr );

    // Create the program object
    programObject = glCreateProgram( );
    if ( programObject == 0 )
        return 0;

    // Attaches a shader object to a program object
    glAttachShader ( programObject, vertexShader );
    glAttachShader ( programObject, fragmentShader );

    // Link the program object
    glLinkProgram ( programObject );

    // Check the link status
    glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );

    if ( !linked )
    {
        GLint infoLen = 0;
        glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );

        if ( infoLen > 1 )
        {
            char* infoLog = (char*)malloc (sizeof(char) * infoLen );
            glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
            LOGE ( "Error linking program:\n%s\n", infoLog );
            free ( infoLog );
        }
        glDeleteProgram ( programObject );
        return GL_FALSE;
    }

    // Free no longer needed shader resources
    glDeleteShader ( vertexShader );
    glDeleteShader ( fragmentShader );

    return programObject;
}

GLESUtils::CameraGLTexImage::CameraGLTexImage(int id, EGLDisplay display, char* bufAddr, buffer_handle_t handle, int share_fd):
    mId(id),
	mShareFd(share_fd),
    eglDisplay(display),
    mBufferAddr(bufAddr),
    mGraphicBuffer(NULL),
    mHandle(handle),
    mEGLImage(EGL_NO_IMAGE_KHR)
{
};

void GLESUtils::CameraGLTexImage::clear() {
    ALOGD("destroy CameraGLTexImage, id: %d", mId);
    mGraphicBuffer = NULL;
    eglDestroyImageKHR(eglDisplay, mEGLImage);
    mEGLImage = EGL_NO_IMAGE_KHR;
};

int GLESUtils::CameraGLTexImage::createTexImage(int width, int height, int format) {
    if (eglDisplay == NULL) return -1;
	ALOGD("createTexImage, w-h: %d-%d, fmt: %d", width, height, format);
	GLuint textureId;
	glGenTextures(1, &textureId);
	glActiveTexture(TEXTURE_GROUP[mId]);

	
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (mHandle == NULL) {
		if (mShareFd != 0) {
			ALOGD("createTexImage, fd: %d", mShareFd);
			mHandle = bufferHandleAlloc(width, height, format, GraphicBuffer::USAGE_HW_TEXTURE);
			private_handle_t* pHnd = (private_handle_t *) mHandle;
			pHnd->share_fd = mShareFd;
			pHnd->ion_hnd = NULL;
			mGraphicBuffer = new GraphicBuffer(width, height, format, //mHandle->format,
                GraphicBuffer::USAGE_HW_TEXTURE, width, (native_handle_t*)mHandle, false);
		} else {
	        mGraphicBuffer = new GraphicBuffer(width, height, format,
	                GraphicBuffer::USAGE_HW_TEXTURE);
		}
    } else {
        mGraphicBuffer = new GraphicBuffer(width, height, format, //mHandle->format,
                GraphicBuffer::USAGE_HW_TEXTURE, width, (native_handle_t*)mHandle, false);

    }

    if (mEGLImage == EGL_NO_IMAGE_KHR) {
		ALOGD("createTexImage, eglimg");
        EGLClientBuffer clientBuffer = (EGLClientBuffer)mGraphicBuffer->getNativeBuffer();
        mEGLImage = eglCreateImageKHR(eglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                clientBuffer, 0);
    }
	checkGlError("createTexImage eglCreateImageKHR");
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)mEGLImage);
	//glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)mEGLImage);
	checkGlError("createTexImage glEGLImageTargetTexture2DOES");

    return 0;
}

int GLESUtils::CameraGLTexImage::updateTexImage(int width, int height, int format) {
	ALOGD("updateTexImage, w-h: %d-%d, fmt: %d, addr: %.8x", width, height, format, mBufferAddr);
    char* buf = NULL;
    status_t err = mGraphicBuffer->lock(GRALLOC_USAGE_SW_WRITE_RARELY, (void**)(&buf));
    if (err != 0) {
        ALOGD("yuvTexBuffer->lock(...) failed: %d\n", err);
        return -1;
    }
#if CPU_INPUT
    if (format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
        memcpy(buf, mBufferAddr, width*height*3/2);
    } else if (format == HAL_PIXEL_FORMAT_RGBA_8888) {
        int* img = (int*)malloc(width * height * sizeof(int));
        YUV420SP2RGBA(width, height, (unsigned char*)img, (unsigned char*)mBufferAddr);
        memcpy(buf, img, width*height*4);
        free(img);
    } else {
        memcpy(buf, mBufferAddr, width*height*3/2);
    }
#elif RGA_INPUT
#if PLATFORM_ARM
    rga_copy_and_scale(width, height, (int)mBufferAddr, RK_FORMAT_YCbCr_420_SP,
                          width, height, (int)buf, RK_FORMAT_YCbCr_420_SP);
#else
    rgaRawDataProcessTiled(width, height, (int)mBufferAddr, RK_FORMAT_YCbCr_420_SP, (int)buf, RK_FORMAT_YCbCr_420_SP);
#endif
#else
	memcpy(buf, mBufferAddr, width*height*3/2);
#endif
    err = mGraphicBuffer->unlock();
    if (err != 0) {
        ALOGD("yuvTexBuffer->unlock() failed: %d\n", err);
        return -1;
    }
    return 0;
}


buffer_handle_t GLESUtils::CameraGLTexImage::bufferHandleAlloc(uint32_t w, uint32_t h, PixelFormat format, uint32_t usage) {

    buffer_handle_t handle;
    int stride;
    GraphicBufferAllocator& allocator = GraphicBufferAllocator::get();
    status_t err = allocator.alloc(w, h, format, usage, &handle, &stride);
    fprintf(stderr, "bufferHandleAlloc status: %d stride = %d, handle = %p\n", err, stride, handle);
    if (err == NO_ERROR) {
        return handle;
    }
    return NULL;
}

void GLESUtils::CameraGLTexImage::bufferHandleFree(buffer_handle_t handle) {

    GraphicBufferAllocator& allocator = GraphicBufferAllocator::get();
    status_t err = allocator.free(handle);
}


void GLESUtils::checkFBO()
{
    // FBO status check
    GLenum status;
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
        case GL_FRAMEBUFFER_COMPLETE:
            LOGE("fbo complete");
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            LOGE("fbo unsupported");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            LOGE("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            LOGE("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_IMG:
            LOGE("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_IMG");
            break;
        default:
            LOGE("Framebuffer Error");
            break;
    }
}

unsigned long GLESUtils::getTickCount() {  
    struct timespec ts;  
    clock_gettime(CLOCK_MONOTONIC, &ts);  
  
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);  
}

void GLESUtils::workBegin()
{
    work_begin = getTickCount();
}

void GLESUtils::workEnd(char* module_name)
{
    work_end = getTickCount() - work_begin;
    //double Time = work_end /((double)getTickFrequency() )* 1000.0;
    LOGE("[%s] TIME = %lf ms \n", module_name, work_end);
}
