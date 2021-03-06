ifneq ($(filter $(BOARD_GPU_DRIVERS), freedreno openfimg),)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Import variables LIBDRM_FREEDRENO_FILES, LIBDRM_FREEDRENO_H_FILES
include $(LOCAL_PATH)/Makefile.sources

LOCAL_MODULE := libdrm_freedreno
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libdrm

LOCAL_SRC_FILES := $(LIBDRM_FREEDRENO_FILES)
LOCAL_EXPORT_C_INCLUDE_DIRS += \
	$(LOCAL_PATH)/freedreno

LOCAL_C_INCLUDES := \
	$(LIBDRM_TOP) \
	$(LIBDRM_TOP)/freedreno \
	$(LIBDRM_TOP)/include/drm

LOCAL_CFLAGS := \
	-DHAVE_LIBDRM_ATOMIC_PRIMITIVES=1

LOCAL_COPY_HEADERS := $(LIBDRM_FREEDRENO_H_FILES)
LOCAL_COPY_HEADERS_TO := freedreno

LOCAL_SHARED_LIBRARIES := \
	libdrm

include $(BUILD_SHARED_LIBRARY)

endif
