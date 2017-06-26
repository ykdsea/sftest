/*
// Copyright(c) 2016 Amlogic Corporation
 */

#define LOG_NDEBUG 0
#define LOG_TAG "CpuTest"

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
#include <utils/String8.h>
#include <utils/Thread.h>
#include <utils/Mutex.h>
#include <utils/Condition.h>

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

#include "WindowSurface.h"
#include "CpuTestRunner.h"

namespace android {

// ---------------------------------------------------------------------------
#define DEFAULT_MAX_BUFFERS  2

CpuTestRunner::CpuTestRunner(TestUtils::ConfigParameters cfgParam) : Thread(false) {
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

CpuTestRunner::~CpuTestRunner() {
}

void CpuTestRunner::onFirstRef() {
    WindowSurface* windowSurface = NULL;
    sp<SurfaceComposerClient> session = NULL;

    windowSurface = new WindowSurface(String8("CpuTestRunner"), mFormat, mFlags, mAlpha, mZorder,
        mWidth, mHeight, mX, mY);
    mSurfaceControl = windowSurface->getSurfaceControl();
    mANW = mSurfaceControl->getSurface();
    mWidth = windowSurface->getWidth();
    mHeight = windowSurface->getHeight();

    mMaxBuffers = DEFAULT_MAX_BUFFERS;

    session = windowSurface->session();
    status_t err = session->linkToComposerDeath(this);
    ALOGE_IF(err, "linkToComposerDeath failed (%s) ", strerror(-err));

    run("CpuTestRunner", PRIORITY_DISPLAY);
}

sp<SurfaceComposerClient> CpuTestRunner::session() const {
    return mSession;
}

void CpuTestRunner::binderDied(const wp<IBinder>&)
{
    // woah, surfaceflinger died!
    ALOGD("SurfaceFlinger died, exiting...");

    // calling requestExit() is not enough here because the Surface code
    // might be blocked on a condition variable that will never be updated.
    kill( getpid(), SIGKILL );
    requestExit();
}

status_t CpuTestRunner::readyToRun() {
    ALOGI("display (%d x %d), format:0x%x, flags:0x%x", mWidth, mHeight, mFormat, mFlags);
    // Config
    configureANW(mANW);

    return NO_ERROR;
}

bool CpuTestRunner::threadLoop()
{
    //status_t err;
    ALOGD("CpuTest Running..");

    // Produce
    const int64_t time = 12345678L;
    uint32_t stride;

    while(mDrawingCnt < mMaxDrawCnt) {
        mDrawingCnt++;
        produceOneFrame(mANW, time, &stride);
        usleep(1000*mFrameIntervalMs);
    }

    TearDown();
    requestExit();
    
    return true;
}

void CpuTestRunner::checkPixel(const CpuConsumer::LockedBuffer &buf,
        uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b) {
    // Ignores components that don't exist for given pixel
    //ALOGE("checkPixel format %d, x=%d, y=%d, r=%d, g=%d, b=%d", 
    //    buf.format, x, y, r, b, g);
    switch(buf.format) {
        case HAL_PIXEL_FORMAT_RAW16: {
            String8 msg;
            uint16_t *bPtr = (uint16_t*)buf.data;
            bPtr += y * buf.stride + x;
            // GRBG Bayer mosaic; only check the matching channel
            switch( ((y & 1) << 1) | (x & 1) ) {
                case 0: // G
                case 3: // G
                    //EXPECT_EQ(g, *bPtr);
                    break;
                case 1: // R
                    //EXPECT_EQ(r, *bPtr);
                    break;
                case 2: // B
                    //EXPECT_EQ(b, *bPtr);
                    break;
            }
            break;
        }
        // ignores g,b
        case HAL_PIXEL_FORMAT_Y8: {
            uint8_t *bPtr = (uint8_t*)buf.data;
            bPtr += y * buf.stride + x;
            //EXPECT_EQ(r, *bPtr) << "at x = " << x << " y = " << y;
            break;
        }
        // ignores g,b
        case HAL_PIXEL_FORMAT_Y16: {
            // stride is in pixels, not in bytes
            //uint16_t *bPtr = ((uint16_t*)buf.data) + y * buf.stride + x;

            //EXPECT_EQ(r, *bPtr) << "at x = " << x << " y = " << y;
            break;
        }
        case HAL_PIXEL_FORMAT_RGBA_8888: {
            const int bytesPerPixel = 4;
            uint8_t *bPtr = (uint8_t*)buf.data;
            bPtr += (y * buf.stride + x) * bytesPerPixel;

            //EXPECT_EQ(r, bPtr[0]) << "at x = " << x << " y = " << y;
            //EXPECT_EQ(g, bPtr[1]) << "at x = " << x << " y = " << y;
            //EXPECT_EQ(b, bPtr[2]) << "at x = " << x << " y = " << y;
            break;
        }
        default: {
            ALOGE("checkPixel format %d, x=%d, y=%d, r=%d, g=%d, b=%d", 
                buf.format, x, y, r, b, g);
            ALOGE("Unknown format for check: %d", buf.format);
            break;
        }
    }
}

uint8_t CpuTestRunner::chooseColorRgb565(int blockX, int blockY, uint8_t channel) {
    const int colorVariations = 3;
    uint8_t color = ((blockX % colorVariations) + (blockY % colorVariations))
                        % (colorVariations) == channel ? 23: 7;

    return color;
}

void CpuTestRunner::fillRgb565Buffer(uint8_t* buf, int w, int h, int stride) {
    const int blockWidth = w > 16 ? w / 16 : 1;
    const int blockHeight = h > 16 ? h / 16 : 1;
    const int bytesPerPixel = 2;
    int gbaValue = mDrawingCnt%3;
    uint8_t r,g,b;

    // stride is in pixels, not in bytes
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            int blockX = (x / blockWidth);
            int blockY = (y / blockHeight);
            if (gbaValue == 0) {
                r = chooseColorRgb565(blockX, blockY, 0);
                g = chooseColorRgb565(blockX, blockY, 1);
                b = chooseColorRgb565(blockX, blockY, 2);
            } else if (gbaValue == 1) {
                r = chooseColorRgb565(blockX, blockY, 1);
                g = chooseColorRgb565(blockX, blockY, 2);
                b = chooseColorRgb565(blockX, blockY, 0);
            } else if (gbaValue == 2) {
                r = chooseColorRgb565(blockX, blockY, 2);
                g = chooseColorRgb565(blockX, blockY, 0);
                b = chooseColorRgb565(blockX, blockY, 1);
            } else {
                ALOGE("chooseColorRgb565 fail, gbaValue: %d", gbaValue);
                r = chooseColorRgb565(blockX, blockY, 0);
                g = chooseColorRgb565(blockX, blockY, 1);
                b = chooseColorRgb565(blockX, blockY, 2);
            }

            buf[(y*stride + x)*bytesPerPixel + 0] = ((r & 0x1f) | ((g & 0x07) >> 0x1f));
            buf[(y*stride + x)*bytesPerPixel + 1] = (((g >> 0x07) & 0x07) | (b << 0x07));;
        }
    }
}

uint8_t CpuTestRunner::chooseColorRgba8888(int blockX, int blockY, uint8_t channel) {
    const int colorVariations = 3;
    uint8_t color = ((blockX % colorVariations) + (blockY % colorVariations))
                        % (colorVariations) == channel ? 191: 63;

    return color;
}

void CpuTestRunner::fillRgba8888Buffer(uint8_t* buf, int w, int h, int stride) {
    const int blockWidth = w > 16 ? w / 16 : 1;
    const int blockHeight = h > 16 ? h / 16 : 1;
    const int bytesPerPixel = 4;
    int gbaValue = mDrawingCnt%3;
    uint8_t r,g,b;

    // stride is in pixels, not in bytes
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            int blockX = (x / blockWidth);
            int blockY = (y / blockHeight);
            if (gbaValue == 0) {
                r = chooseColorRgba8888(blockX, blockY, 0);
                g = chooseColorRgba8888(blockX, blockY, 1);
                b = chooseColorRgba8888(blockX, blockY, 2);
            } else if (gbaValue == 1) {
                r = chooseColorRgba8888(blockX, blockY, 1);
                g = chooseColorRgba8888(blockX, blockY, 2);
                b = chooseColorRgba8888(blockX, blockY, 0);
            } else if (gbaValue == 2) {
                r = chooseColorRgba8888(blockX, blockY, 2);
                g = chooseColorRgba8888(blockX, blockY, 0);
                b = chooseColorRgba8888(blockX, blockY, 1);
            } else {
                ALOGE("fillRgba8888Buffer fail, gbaValue: %d", gbaValue);
                r = chooseColorRgba8888(blockX, blockY, 0);
                g = chooseColorRgba8888(blockX, blockY, 1);
                b = chooseColorRgba8888(blockX, blockY, 2);
            }

            buf[(y*stride + x)*bytesPerPixel + 0] = r;
            buf[(y*stride + x)*bytesPerPixel + 1] = g;
            buf[(y*stride + x)*bytesPerPixel + 2] = b;
            buf[(y*stride + x)*bytesPerPixel + 3] = 255;
        }
    }
}

void CpuTestRunner::checkRgba8888Buffer(const CpuConsumer::LockedBuffer &buf) {
    uint32_t w = buf.width;
    uint32_t h = buf.height;
    const int blockWidth = w > 16 ? w / 16 : 1;
    const int blockHeight = h > 16 ? h / 16 : 1;
    //const int blockRows = h / blockHeight;
    //const int blockCols = w / blockWidth;

    // Top-left square is bright red
    checkPixel(buf, 0, 0, 191, 63, 63);
    checkPixel(buf, 1, 0, 191, 63, 63);
    checkPixel(buf, 0, 1, 191, 63, 63);
    checkPixel(buf, 1, 1, 191, 63, 63);

    // One-right square is bright green
    checkPixel(buf, blockWidth,     0, 63, 191, 63);
    checkPixel(buf, blockWidth + 1, 0, 63, 191, 63);
    checkPixel(buf, blockWidth,     1, 63, 191, 63);
    checkPixel(buf, blockWidth + 1, 1, 63, 191, 63);

    // One-down square is bright green
    checkPixel(buf, 0, blockHeight, 63, 191, 63);
    checkPixel(buf, 1, blockHeight, 63, 191, 63);
    checkPixel(buf, 0, blockHeight + 1, 63, 191, 63);
    checkPixel(buf, 1, blockHeight + 1, 63, 191, 63);

    // One-diag square is bright blue
    checkPixel(buf, blockWidth,     blockHeight, 63, 63, 191);
    checkPixel(buf, blockWidth + 1, blockHeight, 63, 63, 191);
    checkPixel(buf, blockWidth,     blockHeight + 1, 63, 63, 191);
    checkPixel(buf, blockWidth + 1, blockHeight + 1, 63, 63, 191);

    // Test bottom-right pixel
    {
        const int maxBlockX = ((w-1) / blockWidth);
        const int maxBlockY = ((h-1) / blockHeight);
        uint8_t r = chooseColorRgba8888(maxBlockX, maxBlockY, 0);
        uint8_t g = chooseColorRgba8888(maxBlockX, maxBlockY, 1);
        uint8_t b = chooseColorRgba8888(maxBlockX, maxBlockY, 2);
        checkPixel(buf, w-1, h-1, r, g, b);
    }
}

void CpuTestRunner::checkAnyBuffer(const CpuConsumer::LockedBuffer &buf, int format) {
    switch (format) {
        case HAL_PIXEL_FORMAT_RGB_565:
            //checkGreyscaleBuffer<uint16_t>(buf);
            break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            checkRgba8888Buffer(buf);
            break;
    }
}

void CpuTestRunner::TearDown() {
    ALOGD("CpuTest tear down..");
    mANW.clear();
    mSurfaceControl.clear();
}

// Configures the ANativeWindow producer-side interface based on test parameters
void CpuTestRunner::configureANW(const sp<ANativeWindow>& anw) {
    status_t err;
    err = native_window_api_connect(anw.get(), NATIVE_WINDOW_API_CPU);
    if (err != NO_ERROR) {
        ALOGE("connect error: %s", strerror(-err));
        return ;
    }

    err = native_window_set_buffers_dimensions(anw.get(), mWidth, mHeight);
    if (err != NO_ERROR) {
        ALOGE("set_buffers_dimensions error: %s", strerror(-err));
        return ;
    }

    err = native_window_set_buffers_format(anw.get(), mFormat);
    if (err != NO_ERROR) {
        ALOGE("set_buffers_format error: %s", strerror(-err));
        return ;
    }

    err = native_window_set_usage(anw.get(),
            GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_SW_READ_OFTEN);
    if (err != NO_ERROR) {
        ALOGE("set_usage error: %s", strerror(-err));
        return ;
    }

    int minUndequeuedBuffers;
    err = anw.get()->query(anw.get(),
            NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS,
            &minUndequeuedBuffers);
    if (err != NO_ERROR) {
        ALOGE("query error: %s", strerror(-err));
        return ;
    }

    ALOGV("Setting buffer count to %d", mMaxBuffers + 1 + minUndequeuedBuffers);
    err = native_window_set_buffer_count(anw.get(),
            mMaxBuffers + 1 + minUndequeuedBuffers);
    if (err != NO_ERROR) {
        ALOGE("set_buffer_count error:  %s", strerror(-err));
        return ;
    }

}

void CpuTestRunner::produceOneFrame(const sp<ANativeWindow>& anw,
        int64_t timestamp, uint32_t *stride) {
    status_t err;
    ANativeWindowBuffer* anb;
    ALOGV("Dequeue buffer from %p", anw.get());
    err = native_window_dequeue_buffer_and_wait(anw.get(), &anb);
    if (err != NO_ERROR) {
        ALOGE("dequeueBuffer error:  %s", strerror(-err));
        return ;
    }

    if (anb == NULL) {
        ALOGE("anb NULL");
        return ;
    }

    sp<GraphicBuffer> buf(new GraphicBuffer(anb, false));

    *stride = buf->getStride();
    uint8_t* img = NULL;

    ALOGV("Lock buffer from %p for write", anw.get());
    err = buf->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&img));
    if (err != NO_ERROR) {
        ALOGE("lock error: %s", strerror(-err));
        return ;
    }

    switch (mFormat) {
        case HAL_PIXEL_FORMAT_RGB_565:
            fillRgb565Buffer(img, mWidth, mHeight, buf->getStride());
            break;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            fillRgba8888Buffer(img, mWidth, mHeight, buf->getStride());
            break;
        default:
            ALOGE("Unknown pixel format under test.");
            break;
    }
    ALOGV("Unlock buffer from %p", anw.get());
    err = buf->unlock();
    if (err != NO_ERROR) {
        ALOGE("unlock error: %s", strerror(-err));
        return ;
    }

    ALOGV("Set timestamp to %p", anw.get());
    err = native_window_set_buffers_timestamp(anw.get(), timestamp);
    if (err != NO_ERROR) {
        ALOGE("set_buffers_timestamp error: %s", strerror(-err));
        return ;
    }

    ALOGV("Queue buffer to %p", anw.get());
    err = anw->queueBuffer(anw.get(), buf->getNativeBuffer(), -1);
    if (err != NO_ERROR) {
        ALOGE("queueBuffer error: %s", strerror(-err));
        return ;
    }
};
// ---------------------------------------------------------------------------

}
// namespace android
