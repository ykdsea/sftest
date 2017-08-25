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

#include "Gl2TestRunner.h"

namespace android {

// ---------------------------------------------------------------------------
Gl2TestRunner::Gl2TestRunner(TestUtils::ConfigParameters cfgParam) : Thread(false) {
    mFrameIntervalMs = cfgParam.sleepBetweenFramesMs;
    mMaxDrawCnt = cfgParam.drawCounts;
    mFormat = cfgParam.format;
    mAlpha = cfgParam.alpha;
    mZorder = cfgParam.zorder;
    mWidth = cfgParam.width;
    mHeight = cfgParam.heigth;
    mX = cfgParam.x;
    mY = cfgParam.y;

    mFlags = cfgParam.flags;
    mDrawingCnt = 0;
}

Gl2TestRunner::~Gl2TestRunner() {
}

void Gl2TestRunner::onFirstRef() {
    WindowSurface* windowSurface = NULL;
    sp<SurfaceComposerClient> session = NULL;

    windowSurface = new WindowSurface(String8("Gl2TestRunner"), mFormat, mFlags, mAlpha, mZorder, 
        mWidth, mHeight, mX, mY);
    mSurfaceControl = windowSurface->getSurfaceControl();
    mNativeWindowType = (EGLNativeWindowType) ((mSurfaceControl->getSurface()).get());

    session = windowSurface->session();
    status_t err = session->linkToComposerDeath(this);
    ALOGE_IF(err, "linkToComposerDeath failed (%s) ", strerror(-err));

    run("Gl2TestRunner", PRIORITY_DISPLAY);
}

void Gl2TestRunner::binderDied(const wp<IBinder>&)
{
    // woah, surfaceflinger died!
    ALOGD("SurfaceFlinger died, exiting...");

    // calling requestExit() is not enough here because the Surface code
    // might be blocked on a condition variable that will never be updated.
    kill( getpid(), SIGKILL );
    requestExit();
}

status_t Gl2TestRunner::readyToRun() {   
    EGLBoolean returnValue;
    EGLint majorVersion;
    EGLint minorVersion;
    EGLint w, h;
    EGLConfig myConfig = {0};;
    EGLSurface surface;
    EGLContext context;

    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
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
    EGLUtils::checkEglError("eglCreateWindowSurface");
    if (surface == EGL_NO_SURFACE) {
        ALOGE("gelCreateWindowSurface failed.");
        return NO_INIT;
    }
    context = eglCreateContext(display, myConfig, EGL_NO_CONTEXT, context_attribs);
    EGLUtils::checkEglError("eglCreateContext");
    if (context == EGL_NO_CONTEXT) {
        ALOGE("eglCreateContext failed.");
        return NO_INIT;
    }
    //context = eglCreateContext(display, myConfig, NULL, NULL);
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    returnValue = eglMakeCurrent(display, surface, surface, context);
    EGLUtils::checkEglError("eglMakeCurrent", returnValue);
    if (returnValue != EGL_TRUE) {
        ALOGE("eglMakeCurrent failed.");
        return NO_INIT;
    }

    mDisplay = display;
    mContext = context;
    mSurface = surface;
    mWidth = w;
    mHeight = h;

    ALOGI("display (%d x %d), format:0x%x, flags:0x%08x", mWidth, mHeight, mFormat, mFlags);

    ALOGD("EGL version %d.%d", majorVersion, minorVersion);
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    if(!setupGraphics(mWidth, mHeight)) {
        ALOGE("Could not set up graphics.");
        return NO_INIT;
    }

    return NO_ERROR;
}

bool Gl2TestRunner::threadLoop()
{
    ALOGD("Gl2Test Running..");
    
    EGLBoolean returnVal;

    while(mDrawingCnt < mMaxDrawCnt) {
        mDrawingCnt++;
        renderFrame();
        returnVal = eglSwapBuffers(mDisplay, mSurface);
        EGLUtils::checkEglError("eglSwapBuffers", returnVal);
        if (returnVal != EGL_TRUE) {
            break;
        }
        usleep(1000*mFrameIntervalMs);
    }

    ALOGD("Gl2Test tear down..");

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


static const char gVertexShader[] = "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "}\n";
#if 0
static const char gFragmentShader[] = "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";
#else
static const char gFragmentShader[] = "precision mediump float;"
    "uniform vec4 vFragColor;\n"
    "void main() {\n"
    "  gl_FragColor = vFragColor;\n"
    "}\n";
#endif
GLuint Gl2TestRunner::loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    ALOGE("Could not compile shader %d:%s",shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    } 
    return shader;
}

GLuint Gl2TestRunner::createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        ALOGE("loadShader GL_VERTEX_SHADER error");
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        ALOGE("loadShader GL_FRAGMENT_SHADER error");
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    ALOGE("Could not link program:%s", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    } else { 
        ALOGE("glCreateProgram error");
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;
GLuint gvFragColorHandle;

bool Gl2TestRunner::setupGraphics(int w, int h) {
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    ALOGE("glGetAttribLocation(\"vPosition\") = %d", gvPositionHandle);

    gvFragColorHandle = glGetUniformLocation(gProgram, "vFragColor");
    checkGlError("glGetAttribLocation");
    ALOGE("glGetAttribLocation(\"vFragColor\") = %d", gvFragColorHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}

#if 0
const GLfloat gTriangleVertices[] = {
        0.0f, 0.5f,
        -0.5f, -0.5f,
        0.5f, -0.5f };
#else
const GLfloat gTriangleVertices[3][8] = {
    {-0.33f, 1.0f,      // top right
    -1.0f, 1.0f,    // top left
    -1.0f, -1.0f,  // bottom left
    -0.33f, -1.0f}, // bottom right

    {0.33f, 1.0f,      // top right
    -0.33f, 1.0f,    // top left
    -0.33f, -1.0f,  // bottom left
    0.33f, -1.0f}, // bottom right

    {1.0f, 1.0f,      // top right
    0.33f, 1.0f,    // top left
    0.33f, -1.0f,  // bottom left
    1.0f, -1.0f} // bottom right
}; 

const GLushort gVertexIndex[] = { 0, 1, 2, 0, 2, 3 };

#endif

static float colors[][4] = {
    { .85f, .14f, .44f, 1.0f },
    { .91f, .72f, .10f, 1.0f },
    { .04f, .66f, .42f, 1.0f },
    { .84f, .39f, .68f, 1.0f },
    { .38f, .53f, .78f, 1.0f },
};

static size_t g_colorIndex = 0;

float* Gl2TestRunner::genColor() {
    float* color = colors[g_colorIndex];
    g_colorIndex = (g_colorIndex + 1) % NELEMS(colors);
    return color;
}

void Gl2TestRunner::renderFrame() {
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");
    int i;
    for (i=0;  i<3; i++) {
        glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices[i]);
        checkGlError("glVertexAttribPointer");
        glEnableVertexAttribArray(gvPositionHandle);
        checkGlError("glEnableVertexAttribArray");

        glUniform4fv(gvFragColorHandle, 1, genColor());
        checkGlError("glUniform4fv");

        //glDrawArrays(GL_TRIANGLES, 0, 3);
        int nelem = sizeof(gVertexIndex)/sizeof(gVertexIndex[0]);
        glDrawElements(GL_TRIANGLES, nelem, GL_UNSIGNED_SHORT, gVertexIndex);
        checkGlError("glDrawElements");
    }
}

// ---------------------------------------------------------------------------

}
// namespace android
