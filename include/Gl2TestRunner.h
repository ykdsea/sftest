/*
// Copyright(c) 2016 Amlogic Corporation
 */

#ifndef ANDROID_SFTESTRUNNER_H
#define ANDROID_SFTESTRUNNER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Thread.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "TestUtils.h"

class SkBitmap;

namespace android {
#define NELEMS(x) ((int) (sizeof(x) / sizeof((x)[0])))

class Surface;
class SurfaceComposerClient;
class SurfaceControl;

// ---------------------------------------------------------------------------

class Gl2TestRunner : public Thread, public IBinder::DeathRecipient
{
public:
                Gl2TestRunner(TestUtils::ConfigParameters cfgParam);
    virtual     ~Gl2TestRunner();

    sp<SurfaceComposerClient> session() const;

private:
    virtual bool        threadLoop();
    virtual status_t    readyToRun();
    virtual void        onFirstRef();
    virtual void        binderDied(const wp<IBinder>& who);

    GLuint loadShader(GLenum shaderType, const char* pSource) ;
    GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);
    float* genColor();
    bool setupGraphics(int w, int h);
    void renderFrame();

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

void Gl2TestRunner::checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        ALOGE("after %s() glError (0x%x)", op, error);
    }
}


void Gl2TestRunner::printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    ALOGD("GL %s = %s\n", name, v);
}

// ---------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_SFTESTRUNNER_H
