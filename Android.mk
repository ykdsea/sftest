LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=   \
    Main.cpp        \
    GlTestRunner.cpp \
    Gl2TestRunner.cpp \
    CpuTestRunner.cpp \
    lib/WindowSurface.cpp \

LOCAL_MODULE:= sftest
LOCAL_MODULE_TARGET_ARCHS:= arm
LOCAL_MULTILIB := 32


LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_CFLAGS += -Wall -Werror -Wunused -Wunreachable-code

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include \


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libandroidfw \
    libutils \
    libbinder \
    libui \
    libskia \
    libEGL \
    libGLESv1_CM \
    libGLESv2 \
    libgui \


include $(BUILD_EXECUTABLE)
