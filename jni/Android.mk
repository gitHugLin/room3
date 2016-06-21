LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
OPENCV_INSTALL_MODULES:=on
#include $(LOCAL_PATH)/../sdk/native/jni/OpenCV.mk
include /Users/linqi/SDKDir/OpenCV-android-sdk/sdk/native/jni/OpenCV.mk
LOCAL_C_INCLUDES += hardware/rk29/libgralloc_ump/
LOCAL_C_INCLUDES += external/jpeg
LOCAL_C_INCLUDES += hardware/rk29/libon2
LOCAL_C_INCLUDES += hardware/rockchip/libgralloc

LOCAL_SRC_FILES:= \
    GLESUtils.cpp \
    CameraGL.cpp \
    utils.cpp

LOCAL_LDLIBS := -llog -landroid -lGLESv2 -lEGL -lcutils -lui -lutils -lgui
LOCAL_CFLAGS :=  -DEGL_EGLEXT_PROTOTYPES -DGL_GLEXT_PROTOTYPES -DROCKCHIP_GPU_LIB_ENABLE -DHAVE_PTHREADS

LOCAL_MODULE:= libcameragl

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
OPENCV_INSTALL_MODULES:=on
#include $(LOCAL_PATH)/../sdk/native/jni/OpenCV.mk
include /Users/linqi/SDKDir/OpenCV-android-sdk/sdk/native/jni/OpenCV.mk

LOCAL_LDFLAGS := -Wl,--build-id -lskia -llog
LOCAL_CFLAGS :=  -DSK_SUPPORT_LEGACY_SETCONFIG
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += \    $(LOCAL_PATH)/../sdk/native/jni/include
LOCAL_C_INCLUDES += \
    external/skia/include/core \
    external/skia/include/effects \
    external/skia/include/images \
    external/skia/src/ports \
    external/skia/include/utils
LOCAL_SRC_FILES := \
	main.cpp

LOCAL_MODULE:= test-cameragl
LOCAL_SHARED_LIBRARIES := libcameragl libopencv_java3 libskia libcutils libutils
include $(BUILD_EXECUTABLE)
