/*
// Copyright(c) 2016 Amlogic Corporation
 */

#ifndef ANDROID_TEST_UTILS_H
#define ANDROID_TEST_UTILS_H

#include <stdint.h>
#include <stdlib.h>


// ----------------------------------------------------------------------------
namespace android {
// ----------------------------------------------------------------------------

class TestUtils
{
public:
    struct ConfigParameters {
        int       sleepBetweenFramesMs;
        int       drawCounts;
        int       format;
        bool     opaque;
        float    alpha;
        int       zorder;
        int       width;
        int       heigth;
        int       x;
        int       y;
    };
};
// ----------------------------------------------------------------------------
}; // namespace android
// ----------------------------------------------------------------------------

#endif /* ANDROID_TEST_UTILS_H */
