/*
// Copyright(c) 2016 Amlogic Corporation
 */

#ifndef ANDROID_WINDOWSURFACE_H
#define ANDROID_WINDOWSURFACE_H

#include <gui/SurfaceControl.h>

#include <EGL/egl.h>

namespace android {

/*
 * A window that covers the entire display surface.
 *
 * The window is destroyed when this object is destroyed, so don't try
 * to use the surface after that point.
 */
class WindowSurface {
public:
    // Creates the window.
    WindowSurface(const String8& name,// name of the surface
        PixelFormat format, // pixel-format desired
        uint32_t flags = 0,  // usage flags
        float alpha = 1.0f,
        uint32_t zorder = 0x7FFFFFFF,
        uint32_t width = 0,
        uint32_t height = 0,
        uint32_t x = 0,
        uint32_t y = 0
    );

    sp<SurfaceComposerClient> session() const;
    sp<SurfaceControl> getSurfaceControl() const;
    uint32_t getWidth() const;
    uint32_t getHeight() const;

private:
    WindowSurface(const WindowSurface&);
    WindowSurface& operator=(const WindowSurface&);

    uint32_t mWidth;
    uint32_t mHeight;

    sp<SurfaceComposerClient>       mSession;
    sp<SurfaceControl> mSurfaceControl;
};

} // namespace android

#endif /* ANDROID_WINDOWSURFACE_H */
