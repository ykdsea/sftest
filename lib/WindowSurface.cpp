/*
// Copyright(c) 2016 Amlogic Corporation
 */

#include <WindowSurface.h>

#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <ui/DisplayInfo.h>

using namespace android;

WindowSurface::WindowSurface(const String8& name,
        PixelFormat format,
        uint32_t flags,
        float alpha,
        uint32_t zorder,
        uint32_t width,
        uint32_t height,
        uint32_t x,
        uint32_t y) {
    status_t err;

    sp<SurfaceComposerClient> mSession = new SurfaceComposerClient;
    err = mSession->initCheck();
    if (err != NO_ERROR) {
        ALOGE("SurfaceComposerClient::initCheck error: %#x", err);
        return;
    }

    // Get main display parameters.
    sp<IBinder> mainDpy = SurfaceComposerClient::getBuiltInDisplay(
            ISurfaceComposer::eDisplayIdMain);
    DisplayInfo mainDpyInfo;
    err = SurfaceComposerClient::getDisplayInfo(mainDpy, &mainDpyInfo);
    if (err != NO_ERROR) {
        ALOGE("ERROR: unable to get display characteristics");
        return;
    }
    if ((width == 0) || (height == 0)) {
        if (mainDpyInfo.orientation != DISPLAY_ORIENTATION_0 &&
            mainDpyInfo.orientation != DISPLAY_ORIENTATION_180) {
            // rotated
            mWidth = mainDpyInfo.h;
            mHeight = mainDpyInfo.w;
        } else {
            mWidth = mainDpyInfo.w;
            mHeight = mainDpyInfo.h;
        }
    } else {
        mWidth = width;
        mHeight = height;
    }


    sp<SurfaceControl> sc = mSession->createSurface(
            name, mWidth, mHeight, format, flags);
    if (sc == NULL || !sc->isValid()) {
        ALOGE("Failed to create SurfaceControl");
        return;
    }

    SurfaceComposerClient::openGlobalTransaction();
    err = sc->setAlpha(alpha);
    if (err != NO_ERROR) {
        ALOGE("SurfaceComposer::setAlpha error: %#x", err);
        return;
    }

    err = sc->setLayer(zorder);
    if (err != NO_ERROR) {
        ALOGE("SurfaceComposer::setLayer error: %#x", err);
        return;
    }

    err = sc->setPosition(x, y);
    if (err != NO_ERROR) {
        ALOGE("SurfaceComposer::setPosition error: %#x", err);
        return;
    }

    err = sc->show();
    if (err != NO_ERROR) {
        ALOGE("SurfaceComposer::show error: %#x", err);
        return;
    }
    SurfaceComposerClient::closeGlobalTransaction();

    mSurfaceControl = sc;
}

sp<SurfaceComposerClient> WindowSurface::session() const {
    return mSession;
}

sp<SurfaceControl> WindowSurface::getSurfaceControl() const {
    return mSurfaceControl;
}

uint32_t WindowSurface::getWidth() const {
    return mWidth;
}

uint32_t WindowSurface::getHeight() const {
    return mHeight;
}



