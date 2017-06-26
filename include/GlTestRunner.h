/*
// Copyright(c) 2016 Amlogic Corporation
 */

#ifndef ANDROID_GLTESTRUNNER_H
#define ANDROID_GLTESTRUNNER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Thread.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "TestUtils.h"

class SkBitmap;

namespace android {

class Surface;
class SurfaceComposerClient;
class SurfaceControl;

// ---------------------------------------------------------------------------

class GlTestRunner : public Thread, public IBinder::DeathRecipient
{
public:
                GlTestRunner(TestUtils::ConfigParameters cfgParam);
    virtual     ~GlTestRunner();

    sp<SurfaceComposerClient> session() const;

private:
    virtual bool        threadLoop();
    virtual status_t    readyToRun();
    virtual void        onFirstRef();
    virtual void        binderDied(const wp<IBinder>& who);

    void gluLookAt(float eyeX, float eyeY, float eyeZ,
        float centerX, float centerY, float centerZ, float upX, float upY,
        float upZ);
    void init_scene(void);
    void render(int quads);
    void create_texture(void);

    static inline void checkGlError(const char* op);
    static inline void printGLString(const char *name, GLenum s);

    sp<SurfaceComposerClient>       mSession;
    int         mFrameIntervalMs;
    int         mMaxDrawCnt;
    PixelFormat         mFormat;
    uint32_t         mFlags;
    float       mAlpha;
    uint32_t         mZorder;
    int         mDrawingCnt;

    int         mWidth;
    int         mHeight;
    int         mX;
    int         mY;
    EGLDisplay  mDisplay;
    EGLDisplay  mContext;
    EGLDisplay  mSurface;

    sp<SurfaceControl> mSurfaceControl;
    EGLNativeWindowType mNativeWindowType;
};

void GlTestRunner::checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        ALOGE("after %s() glError (0x%x)", op, error);
    }
}

void GlTestRunner::printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    ALOGD("GL %s = %s\n", name, v);
}

// ---------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_GLTESTRUNNER_H