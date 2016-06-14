#include <CameraGL.h>
#include "opencv2/opencv.hpp"
#include <stdlib.h>
#include "log.h"

#include <fcntl.h>
#include <external/skia/include/core/SkImageEncoder.h>
#include <external/skia/include/core/SkBitmap.h>
#include <external/skia/include/core/SkData.h>
#include <external/skia/include/core/SkStream.h>

CameraGL* mCameraGL;
struct cv_fimc_buffer* m_buffers_capture;
int width;
int height;

using namespace cv;
using namespace std;

int SkSavePng(int width, int height, void* result) {
	int fd = open("/data/local/result.png", O_WRONLY | O_CREAT | O_TRUNC, 0664);
	 if (fd == -1) {
		 LOGE("Error opening file: /data/local/result.png");
		 return -1;
	 }
	 
	 const SkImageInfo info = SkImageInfo::Make(width, height, kRGBA_8888_SkColorType,
															kPremul_SkAlphaType);
	 SkBitmap b;
	 b.installPixels(info, (void*)(result), width*4);
	 SkDynamicMemoryWStream stream;
	 SkImageEncoder::EncodeStream(&stream, b,
			 SkImageEncoder::kPNG_Type, SkImageEncoder::kDefaultQuality);
	 SkData* streamData = stream.copyToData();
	 write(fd, streamData->data(), streamData->size());
	 streamData->unref();
	 close(fd);
}

int main () {
	Mat rgb, yv12;
	m_buffers_capture = new cv_fimc_buffer();
	mCameraGL = new CameraGL();

	if (!mCameraGL->initialized) {
		LOGE("initializing...");
		Mat bayer;
        bayer = imread( "/sdcard/night.pgm",0);
        cvtColor(bayer, rgb, CV_BayerBG2RGB);
		width = rgb.cols;
		height = rgb.rows;
		mCameraGL->init(NULL, width, height);
	}

	int result;
	LOGE("updating...");
	cvtColor(rgb, yv12, COLOR_RGB2YUV_YV12);
	m_buffers_capture->start = yv12.data;
	m_buffers_capture->share_fd = 0;
	m_buffers_capture->length = width * height * 3 / 2;
	m_buffers_capture->handle = NULL;
	mCameraGL->update(m_buffers_capture);

	LOGE("processing...");
	mCameraGL->process(&result, OUTPUT_FMT_RGBA);
	LOGD("precess result: %.8x", result);
	//SkSavePng(width, height, (void*)result);
	Mat rgba(height, width, CV_8UC4, (void*)result);
	imwrite("/data/local/result.jpg", rgba);

	LOGE("destroying...");
	mCameraGL->destroy();

	return 0;
}

