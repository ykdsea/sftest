/*
// Copyright(c) 2016 Amlogic Corporation
 */

#ifndef ANDROID_CPUTESTRUNNER_H
#define ANDROID_CPUTESTRUNNER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Thread.h>

#include <gui/CpuConsumer.h>
#include <ui/GraphicBuffer.h>
#include "TestUtils.h"

namespace android {

class Surface;
class SurfaceComposerClient;
class SurfaceControl;

// ---------------------------------------------------------------------------

class CpuTestRunner : public Thread, public IBinder::DeathRecipient
{
public:
                CpuTestRunner(TestUtils::ConfigParameters cfgParam);
    virtual     ~CpuTestRunner();

    sp<SurfaceComposerClient> session() const;

private:
    virtual bool        threadLoop();
    virtual status_t    readyToRun();
    virtual void        onFirstRef();
    virtual void        binderDied(const wp<IBinder>& who);

    void checkPixel(const CpuConsumer::LockedBuffer &buf,
        uint32_t x, uint32_t y, uint32_t r, uint32_t g=0, uint32_t b=0);
    uint8_t chooseColorRgb565(int blockX, int blockY, uint8_t channel);
    void fillRgb565Buffer(uint8_t* buf, int w, int h, int stride);
    uint8_t chooseColorRgba8888(int blockX, int blockY, uint8_t channel);
    void fillRgba8888Buffer(uint8_t* buf, int w, int h, int stride);
    void checkRgba8888Buffer(const CpuConsumer::LockedBuffer &buf);
    void checkAnyBuffer(const CpuConsumer::LockedBuffer &buf, int format);
    void SetUp();
    void TearDown();
    void configureANW(const sp<ANativeWindow>& anw);
    void produceOneFrame(const sp<ANativeWindow>& anw,
        int64_t timestamp, uint32_t *stride);

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
    int         mMaxBuffers;

    sp<SurfaceControl> mSurfaceControl;
    sp<ANativeWindow> mANW;
};

// ---------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_CPUTESTRUNNER_H
