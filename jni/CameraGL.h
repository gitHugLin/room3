#include <jni.h>
#include "string.h"
#include "assert.h"
#include <system/window.h>
#include <hardware/gralloc.h>

#ifndef CAMERA_GL__H
#define CAMERA_GL__H

enum OUTPUT_FMT {
	OUTPUT_FMT_CV_MAT,
	OUTPUT_FMT_YUV,
	OUTPUT_FMT_RGBA,
	OUTPUT_NONE
};

struct cv_fimc_buffer {
	void	*start;
	int share_fd;
	size_t	length;
	int stride;
	size_t	bytesused;
	buffer_handle_t handle;
};

class CameraGL {
	public:
		bool initialized;
		CameraGL();
		~CameraGL();
		void init(alloc_device_t *m_alloc_dev, int width, int height);
		void update(struct cv_fimc_buffer *m_buffers_capture);
		long process(int* targetAddr, int mode = OUTPUT_NONE);
		void destroy();
	private:
		void getImageUnderDir(char *path, char *suffix);

};
#endif //CAMERA_GL__H

