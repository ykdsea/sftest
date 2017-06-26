/*
// Copyright(c) 2016 Amlogic Corporation
 */
#define LOG_TAG "SfTest"
#include <stdint.h>
#include <sys/types.h>
#include <sys/resource.h>

#include <gui/GraphicBufferAlloc.h>
#include <gui/Surface.h>
#include <gui/SurfaceControl.h>
#include <gui/GLConsumer.h>
#include <gui/Surface.h>
#include <ui/Fence.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/Trace.h>

#include <EGL/egl.h>

#include <math.h>
#include <getopt.h>

#include "CpuTestRunner.h"
#include "GlTestRunner.h"
#include "Gl2TestRunner.h"
#include "TestUtils.h"

using namespace android;


static TestUtils::ConfigParameters g_CfgParam;
static int g_DrawMode = 2;        //0-cpu draw, 1-gl draw, 2-gl2 draw
                                                  //default:gl2 draw

void sfTests() {
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();
    if ((g_CfgParam.width == 0) || (g_CfgParam.heigth == 0)) {
        ALOGD("sfTests: frame interval time(%dms), draw counts(%d), format(%d), opaque(%d),"
            " alpha(%.3f), zorder(%d), display(default)",
            g_CfgParam.sleepBetweenFramesMs, g_CfgParam.drawCounts, g_CfgParam.format,
            g_CfgParam.opaque, g_CfgParam.alpha, g_CfgParam.zorder);
    } else {
        ALOGD("sfTests: frame interval time(%dms), draw counts(%d), format(%d), opaque(%d),"
            " alpha(%.3f), zorder(%d), display(%dx%d), position(%dx%d)",
            g_CfgParam.sleepBetweenFramesMs, g_CfgParam.drawCounts, g_CfgParam.format,
            g_CfgParam.opaque, g_CfgParam.alpha, g_CfgParam.zorder, g_CfgParam.width, g_CfgParam.heigth,
            g_CfgParam.x, g_CfgParam.y);
    }

    switch(g_DrawMode) {
        case 0: {
            ALOGD("Software draw start...");
            sp<CpuTestRunner> sf = new CpuTestRunner(g_CfgParam);
            break;
        }

        case 1: {
            ALOGD("GL draw start...");
            sp<GlTestRunner> sf = new GlTestRunner(g_CfgParam);
            break;
        }

        case 2: {
            ALOGD("GL2 draw start...");
            sp<Gl2TestRunner> sf = new Gl2TestRunner(g_CfgParam);
            break;
        }
        default: {

            break;
        }
    }

    IPCThreadState::self()->joinThreadPool();

    return;
}

// Print the command usage help to stderr.
static void showHelp(const char *cmd) {
    fprintf(stderr, "usage: %s [options]\n", cmd);
    fprintf(stderr, "options include:\n"                    
                    "  -s           force software draw to a window\n"
                    "  -o           force Opaque\n"
                    "  -c  N        config draw counters\n"
                    "  -i  N        sleep for N ms between per frame\n"
                    "  -f  N        format(default:rgba8888):1-rgba8888, 4-rgb565\n"
                    "  -z  N        Z order value(default:0x7FFFFFFF, always on top):1-0x7FFFFFFF\n"
                    "  --gl         force gl draw\n"
                    "  --alpha N    config alpha value(default:1.0), 0.0-1.0\n"
                    "  --size       config window size:widthxheight, such as 1920x1080\n"
                    "  --position   config window position:XxY, such as 32x32\n"
                    "  --help       print this helpful message and exit\n"
            );
}

int calWidthAndHeight(char* wxh, int* width, int* heigth) {
    //fprintf(stderr, "-size %s\n", wxh);
    char* xPosition = strstr(wxh,"x");
    if (xPosition == NULL) {
        return -1;
    }
    int wxhLen = strlen(wxh);
    int widthLen = wxhLen-strlen(xPosition) ;
    //int heigthLen = strlen(xPosition)-1;
    //fprintf(stderr, "-size wxhLen:%d, widthLen:%d, heigthLen:%d\n", wxhLen, widthLen, heigthLen);
    char widthStr[32] = {0};
    char heigthStr[32] = {0};
    memcpy(widthStr, wxh, widthLen);
    memcpy(heigthStr, wxh+widthLen+1, widthLen);
    *width = atoi(widthStr);
    *heigth = atoi(heigthStr);
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 2 && 0 == strcmp(argv[1], "--help")) {
        showHelp(argv[0]);
        exit(0);
    }

    g_CfgParam.sleepBetweenFramesMs = 1000;
    g_CfgParam.drawCounts = 100000;
    g_CfgParam.format = 1;
    g_CfgParam.opaque = false;
    g_CfgParam.zorder = INT_MAX;
    g_CfgParam.alpha = 1.0f;
    g_CfgParam.width = 0;
    g_CfgParam.heigth = 0;
    g_CfgParam.x = 0;
    g_CfgParam.y = 0;

    for (;;) {
        int ret;
        int option_index = 0;
        int intArg = 0;
        static struct option long_options[] = {
            {"help",     no_argument, 0,  0 },
            {"gl",     no_argument, 0,  0 },
            {"size",     required_argument, 0,  0 },
            {"position",     required_argument, 0,  0 },
            {"alpha",     required_argument, 0,  0 },
            {     0,               0, 0,  0 }
        };

        ret = getopt_long(argc, argv, "soi:z:c:f:",
                          long_options, &option_index);

        if (ret < 0) {
            break;
        }
        //ALOGD("options:%d", ret);

        switch(ret) {            
            case 's':
                g_DrawMode = 0;
            break;

            case 'o':
                g_CfgParam.opaque = true;
            break;

            case 'c':
                intArg = atoi(optarg);
                if (intArg <= 0) {
                    fprintf(stderr, "c %d is invalid!!!\n", intArg);
                    exit(2);
                }
                g_CfgParam.drawCounts = intArg;
            break;

            case 'i':
                intArg = atoi(optarg);
                if (intArg <= 0) {
                    fprintf(stderr, "i %d is invalid!!!\n", intArg);
                    exit(2);
                }
                g_CfgParam.sleepBetweenFramesMs = intArg;
            break;

            case 'f':
                intArg = atoi(optarg);
                if ((intArg != 1)  &&  (intArg != 4)){
                    fprintf(stderr, "f %d is invalid!!!\n", intArg);
                    exit(2);
                }
                g_CfgParam.format = intArg;
            break;

            case 'z':
                intArg = atoi(optarg);
                if (intArg <= 0){
                    fprintf(stderr, "z %d is invalid!!!\n", intArg);
                    exit(2);
                }
                g_CfgParam.zorder = intArg;
            break;

            case 0:
                if (!strcmp(long_options[option_index].name, "help")) {
                    showHelp(argv[0]);
                    exit(0);
                } else if (!strcmp(long_options[option_index].name, "gl")) {
                    g_DrawMode = 1;
                } else if (!strcmp(long_options[option_index].name, "size")) {
                    if (-1 == calWidthAndHeight(optarg, &g_CfgParam.width, &g_CfgParam.heigth)) {
                        fprintf(stderr, "size parameters:(%s) format is invalid!\n", optarg);
                        exit(2);
                    }
                } else if (!strcmp(long_options[option_index].name, "position")) {
                    if (-1 == calWidthAndHeight(optarg, &g_CfgParam.x, &g_CfgParam.y)) {
                        fprintf(stderr, "position parameters:(%s) format is invalid!\n", optarg);
                        exit(2);
                    }
                } else if (!strcmp(long_options[option_index].name, "alpha")) {
                    sscanf(optarg, "%f", &g_CfgParam.alpha);
                    if ((g_CfgParam.alpha < 0.0f) || (g_CfgParam.alpha > 1.0f)) {
                        fprintf(stderr, "alpha parameters:(%.3f) is invalid!\n", g_CfgParam.alpha);
                        exit(2);
                    }
                }
            break;

            default:
                showHelp(argv[0]);
                exit(2);
        }
    }

    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);

    sfTests();

    return 1;
}
