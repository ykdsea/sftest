/*
// Copyright(c) 2016 Amlogic Corporation
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "SfTest"

#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <utils/misc.h>
#include <signal.h>
#include <time.h>

#include <cutils/properties.h>

#include <androidfw/AssetManager.h>
#include <binder/IPCThreadState.h>
#include <utils/Atomic.h>
#include <utils/Errors.h>
#include <utils/Log.h>

#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <ui/Region.h>
#include <ui/DisplayInfo.h>

#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

// TODO: Fix Skia.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkImageDecoder.h>
#pragma GCC diagnostic pop

#include "EGLUtils.h"
#include "WindowSurface.h"

#include "GlTestRunner.h"

namespace android {

// ---------------------------------------------------------------------------
GlTestRunner::GlTestRunner(TestUtils::ConfigParameters cfgParam) : Thread(false) {
    mFrameIntervalMs = cfgParam.sleepBetweenFramesMs;
    mMaxDrawCnt = cfgParam.drawCounts;
    mFormat = cfgParam.format;
    mAlpha = cfgParam.alpha;
    mZorder= cfgParam.zorder;
    mWidth = cfgParam.width;
    mHeight = cfgParam.heigth;
    mX = cfgParam.x;
    mY = cfgParam.y;

    mFlags = 0;
    if (cfgParam.opaque) {
        mFlags |= ISurfaceComposerClient::eOpaque;
    }
    mDrawingCnt = 0;
}

GlTestRunner::~GlTestRunner() {
}

void GlTestRunner::onFirstRef() {
    WindowSurface* windowSurface = NULL;
    sp<SurfaceComposerClient> session = NULL;

    windowSurface = new WindowSurface(String8("GlTestRunner"), mFormat, mFlags, mAlpha, mZorder,
        mWidth, mHeight, mX, mY);
    mSurfaceControl = windowSurface->getSurfaceControl();
    mNativeWindowType = (EGLNativeWindowType) ((mSurfaceControl->getSurface()).get());

    session = windowSurface->session();
    status_t err = session->linkToComposerDeath(this);
    ALOGE_IF(err, "linkToComposerDeath failed (%s) ", strerror(-err));

    run("GlTestRunner", PRIORITY_DISPLAY);
}

void GlTestRunner::binderDied(const wp<IBinder>&)
{
    // woah, surfaceflinger died!
    ALOGD("SurfaceFlinger died, exiting...");

    // calling requestExit() is not enough here because the Surface code
    // might be blocked on a condition variable that will never be updated.
    kill( getpid(), SIGKILL );
    requestExit();
}

status_t GlTestRunner::readyToRun() {
    EGLBoolean returnValue;
    EGLint majorVersion;
    EGLint minorVersion;
    EGLint w, h;
    EGLConfig myConfig = {0};;
    EGLSurface surface;
    EGLContext context;
    
    EGLint s_configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE };
    

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    returnValue = eglInitialize(display, &majorVersion, &minorVersion);
    EGLUtils::checkEglError("eglInitialize", returnValue);

#if 0
    if (!EGLUtils::printEGLConfigurations(display)) {
        ALOGE("printEGLConfigurations failed");
        return NO_INIT;
    }
#endif

    returnValue = EGLUtils::selectConfigForNativeWindow(display, s_configAttribs, mNativeWindowType, &myConfig);
    if (returnValue) {
        ALOGE("EGLUtils::selectConfigForNativeWindow() returned %d", returnValue);
        return NO_INIT;
    }

#if 0
    EGLUtils::printEGLConfiguration(display, myConfig);    
#endif

    surface = eglCreateWindowSurface(display, myConfig, mNativeWindowType, NULL);
    context = eglCreateContext(display, myConfig, NULL, NULL);
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
        return NO_INIT;

    mDisplay = display;
    mContext = context;
    mSurface = surface;
    mWidth = w;
    mHeight = h;

    ALOGI("display (%d x %d), format:0x%x, flags:0x%x", mWidth, mHeight, mFormat, mFlags);

    ALOGD("EGL version %d.%d", majorVersion, minorVersion);
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    init_scene();
    
    return NO_ERROR;
}

bool GlTestRunner::threadLoop()
{
    ALOGD("GlTest Running..");

    while(mDrawingCnt < mMaxDrawCnt) {
        mDrawingCnt++;
        create_texture();
        render(60*10);
        usleep(1000*mFrameIntervalMs);
    }

    ALOGD("GlTest tear down..");

    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(mDisplay, mContext);
    eglDestroySurface(mDisplay, mSurface);
    //mNativeWindowType.clear();
    mSurfaceControl.clear();
    eglTerminate(mDisplay);
    IPCThreadState::self()->stopProcess();
    
    requestExit();
    
    return true;
}

void GlTestRunner::gluLookAt(float eyeX, float eyeY, float eyeZ,
        float centerX, float centerY, float centerZ, float upX, float upY,
        float upZ)
{
    // See the OpenGL GLUT documentation for gluLookAt for a description
    // of the algorithm. We implement it in a straightforward way:

    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize f
    float rlf = 1.0f / sqrtf(fx*fx + fy*fy + fz*fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf;

    // Normalize up
    float rlup = 1.0f / sqrtf(upX*upX + upY*upY + upZ*upZ);
    upX *= rlup;
    upY *= rlup;
    upZ *= rlup;

    // compute s = f x up (x means "cross product")

    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;

    // compute u = s x f
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    float m[16] ;
    m[0] = sx;
    m[1] = ux;
    m[2] = -fx;
    m[3] = 0.0f;

    m[4] = sy;
    m[5] = uy;
    m[6] = -fy;
    m[7] = 0.0f;

    m[8] = sz;
    m[9] = uz;
    m[10] = -fz;
    m[11] = 0.0f;

    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;

    glMultMatrixf(m);
    checkGlError("glMultMatrixf");
    glTranslatef(-eyeX, -eyeY, -eyeZ);
    checkGlError("glTranslatef");
}

void GlTestRunner::init_scene(void)
{
    glDisable(GL_DITHER);
    checkGlError("glDisable");
    glEnable(GL_CULL_FACE);
    checkGlError("glEnable");

    float ratio = mWidth/ mHeight;
    glViewport(0, 0, mHeight, mHeight);
    checkGlError("glViewport");

    glMatrixMode(GL_PROJECTION);
    checkGlError("glMatrixMode");
    glLoadIdentity();
    checkGlError("glLoadIdentity");
    glFrustumf(-ratio, ratio, -1, 1, 1, 10);
    checkGlError("glFrustumf");

    glMatrixMode(GL_MODELVIEW);
    checkGlError("glMatrixMode");
    glLoadIdentity();
    checkGlError("glLoadIdentity");
    gluLookAt(
            0, 0, 3,  // eye
            0, 0, 0,  // center
            0, 1, 0); // up

    glEnable(GL_TEXTURE_2D);
    checkGlError("glEnable");
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    checkGlError("glEnableClientState");
}

static size_t g_colorIndex = 0;

void GlTestRunner::create_texture(void)
{
    GLuint texture;
    const unsigned int on = 0xff0000ff;
    const unsigned int ox = 0x00ffff00;
    const unsigned int off = 0xffffffff;
    const unsigned int pixels1[] =
    {
            on, off, on, off, on, off, on, off,
            off, on, off, on, off, on, off, on,
            on, off, on, off, on, off, on, off,
            off, on, off, on, off, on, off, on,
            on, off, on, off, on, off, on, off,
            off, on, off, on, off, on, off, on,
            on, off, on, off, on, off, on, off,
            off, on, off, on, off, on, off, on,
    };
    const unsigned int pixels2[] =
    {
            ox, off, ox, off, ox, off, ox, off,
            off, ox, off, ox, off, ox, off, ox,
            ox, off, ox, off, ox, off, ox, off,
            off, ox, off, ox, off, ox, off, ox,
            ox, off, ox, off, ox, off, ox, off,
            off, ox, off, ox, off, ox, off, ox,
            ox, off, ox, off, ox, off, ox, off,
            off, ox, off, ox, off, ox, off, ox,
    };
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    g_colorIndex = (g_colorIndex+1)%2;
    if (g_colorIndex == 0) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels1);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels2);
    }
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    checkGlError("create_texture");
}

void GlTestRunner::render(int quads)
{
    int i;

    const GLfloat vertices[] = {
            -1,  -1,  0,
             1,  -1,  0,
             1,   1,  0,
            -1,   1,  0
    };

    const GLfixed texCoords[] = {
            0,            0,
            0x10000,    0,
            0x10000,    0x10000,
            0,            0x10000
    };

    const GLushort quadIndices[] = { 0, 1, 2,  0, 2, 3 };


    GLushort* indices = (GLushort*)malloc(quads*sizeof(quadIndices));
    for (i=0 ; i<quads ; i++)
        memcpy(indices+(sizeof(quadIndices)/sizeof(indices[0]))*i, quadIndices, sizeof(quadIndices));

    glVertexPointer(3, GL_FLOAT, 0, vertices);
    checkGlError("glVertexPointer");
    glTexCoordPointer(2, GL_FIXED, 0, texCoords);
    checkGlError("glTexCoordPointer");

    // make sure to do a couple eglSwapBuffers to make sure there are
    // no problems with the very first ones (who knows)

    glClearColor(1.0, 1.0, 1.0, 1.0);

    int nelem = sizeof(quadIndices)/sizeof(quadIndices[0]);

    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDrawElements(GL_TRIANGLES, nelem*quads, GL_UNSIGNED_SHORT, indices);
    checkGlError("glDrawElements");
    eglSwapBuffers(mDisplay, mSurface);

    free(indices);
}

// ---------------------------------------------------------------------------

}
// namespace android
