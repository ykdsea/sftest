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
        float    alpha;
        int       zorder;
        int       width;
        int       heigth;
        int       x;
        int       y;
        uint32_t flags;
        bool    useBuffer;
        uint32_t buffer;
    };
};
// ----------------------------------------------------------------------------
}; // namespace android
// ----------------------------------------------------------------------------

#endif /* ANDROID_TEST_UTILS_H */
