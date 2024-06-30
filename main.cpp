#include <SRMCore.h>
#include <SRMDevice.h>
#include <SRMConnector.h>
#include <SRMConnectorMode.h>
#include <SRMListener.h>

#include <SRMList.h>
#include <SRMLog.h>

#include <GLES2/gl2.h>

#include <fcntl.h>
#include <unistd.h>

#include "include/gpu/gl/GrGLInterface.h"
#include "include/gpu/gl/GrGLTypes.h"
#include "include/gpu/GrDirectContext.h"
#include "include/gpu/GrBackendSurface.h"
#include "include/core/SkImage.h"
#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include "include/gpu/gl/GrGLAssembleInterface.h"

#define BALL_SPEED 10.0
#define BALLS_N 20

struct Skia
{
    sk_sp<GrDirectContext> context;
    sk_sp<SkSurface> gpuSurface;
    SkCanvas *gpuCanvas;
    double t = 0.f;
};

struct Ball
{
    double x = 0.0;
    double y = 0.0;
    double dx = 2.0;
    double dy = 2.0;
};

struct ConnectorData
{
    Skia skia;
    Ball balls[BALLS_N];
};

static int openRestricted(const char *path, int flags, void *userData)
{
    SRM_UNUSED(userData);
    return open(path, flags);
}

static void closeRestricted(int fd, void *userData)
{
    SRM_UNUSED(userData);
    close(fd);
}

static SRMInterface srmInterface
{
    .openRestricted = &openRestricted,
    .closeRestricted = &closeRestricted
};

static void initializeGL(SRMConnector *connector, void *userData)
{
    ConnectorData *data = (ConnectorData*)userData;
    SRMConnectorMode *mode = srmConnectorGetCurrentMode(connector);

    for (int i = 0; i < BALLS_N; i++)
    {
        data->balls[i].x = rand() % srmConnectorModeGetWidth(mode);
        data->balls[i].y = rand() % srmConnectorModeGetHeight(mode);
        data->balls[i].dx = double(rand() % 10)/10.f;
        data->balls[i].dy = double(rand() % 10)/10.f;
    }

    auto interface = GrGLMakeAssembledInterface(nullptr, (GrGLGetProc)*[](void *, const char *p) -> void * {
        return (void *)eglGetProcAddress(p);
    });

    GrContextOptions options;
    options.fShaderCacheStrategy = GrContextOptions::ShaderCacheStrategy::kSkSL;
    options.fAvoidStencilBuffers = true;
    options.fPreferExternalImagesOverES3 = true;
    options.fDisableGpuYUVConversion = true;
    options.fReduceOpsTaskSplitting = GrContextOptions::Enable::kNo;
    options.fReducedShaderVariations = false;

    data->skia.context = GrDirectContext::MakeGL(interface, options);

    if (!data->skia.context.get())
    {
        SRMFatal("Failed to create context.");
        exit(1);
    }

    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = 0;
    fbInfo.fFormat = GL_RGB8_OES;

    GrBackendRenderTarget target(
        srmConnectorModeGetWidth(mode),
        srmConnectorModeGetHeight(mode),
        0, 0,
        fbInfo);

    sk_sp<SkColorSpace> colorSpace = SkColorSpace::MakeSRGBLinear();
    SkSurfaceProps skSurfaceProps(0, kUnknown_SkPixelGeometry);

    data->skia.gpuSurface = SkSurface::MakeFromBackendRenderTarget(
        data->skia.context.get(),
        target,
        GrSurfaceOrigin::kBottomLeft_GrSurfaceOrigin,
        SkColorType::kRGB_888x_SkColorType,
        colorSpace,
        &skSurfaceProps);

    if (!data->skia.gpuSurface.get())
    {
        SRMFatal("SkSurface::MakeRenderTarget returned nullptr.");
        exit(1);
    }

    data->skia.gpuCanvas = data->skia.gpuSurface->getCanvas();
    srmConnectorRepaint(connector);
}

static void paintGL(SRMConnector *connector, void *userData)
{
    ConnectorData *data = (ConnectorData*)userData;
    SRMConnectorMode *mode = srmConnectorGetCurrentMode(connector);

    double scale = 1.f + (1.0 + cos(data->skia.t));
    data->skia.gpuCanvas->clear(SK_ColorBLUE);
    SkPaint paint;
    SkMatrix m;
    m.setScale(scale, scale);
    data->skia.gpuCanvas->setMatrix(m);
    paint.setColor(SK_ColorWHITE);

    for (int i = 0; i < BALLS_N; i++)
    {
        data->skia.gpuCanvas->drawCircle(data->balls[i].x, data->balls[i].y, 10, paint);
        data->skia.gpuCanvas->flush();
        data->balls[i].x += data->balls[i].dx;
        data->balls[i].y += data->balls[i].dy;

        if (data->balls[i].x < 0)
            data->balls[i].dx = BALL_SPEED;

        if (data->balls[i].x > srmConnectorModeGetWidth(mode)/scale)
            data->balls[i].dx = -BALL_SPEED;

        if (data->balls[i].y < 0)
            data->balls[i].dy = BALL_SPEED;

        if (data->balls[i].y > srmConnectorModeGetHeight(mode)/scale)
            data->balls[i].dy = -BALL_SPEED;
    }

    srmConnectorRepaint(connector);
    data->skia.t += 0.005f;
}

static void resizeGL(SRMConnector */*connector*/, void */*userData*/) {}
static void pageFlipped(SRMConnector */*connector*/, void */*userData*/) {}
static void uninitializeGL(SRMConnector */*connector*/, void *userData)
{
    ConnectorData *data = (ConnectorData*)userData;

    /* TODO: Free Skia stuff */

    delete data;
}

static SRMConnectorInterface connectorInterface =
{
    .initializeGL = &initializeGL,
    .paintGL = &paintGL,
    .pageFlipped = &pageFlipped,
    .resizeGL = &resizeGL,
    .uninitializeGL = &uninitializeGL
};

static void initConnector(SRMConnector *connector)
{
    ConnectorData *data { new ConnectorData };

    if (!srmConnectorInitialize(connector, &connectorInterface, data))
    {
        SRMError("[srm-skia] Failed to initialize connector %s.",
                 srmConnectorGetModel(connector));

        delete data;
    }
}

static void connectorPluggedEventHandler(SRMListener */*listener*/, SRMConnector *connector)
{
    initConnector(connector);
}

static void connectorUnpluggedEventHandler(SRMListener *listener, SRMConnector *connector)
{
    SRM_UNUSED(listener);
    SRM_UNUSED(connector);
}

int main(void)
{
    SRMCore *core = srmCoreCreate(&srmInterface, NULL);

    if (!core)
    {
        SRMFatal("[srm-skia] Failed to initialize SRM core.");
        return 1;
    }

    srmCoreAddConnectorPluggedEventListener(core, &connectorPluggedEventHandler, NULL);
    srmCoreAddConnectorUnpluggedEventListener(core, &connectorUnpluggedEventHandler, NULL);

    SRMListForeach (deviceIt, srmCoreGetDevices(core))
    {
        SRMDevice *device = (SRMDevice*)srmListItemGetData(deviceIt);

        SRMListForeach (connectorIt, srmDeviceGetConnectors(device))
        {
            SRMConnector *connector = (SRMConnector*)srmListItemGetData(connectorIt);

            if (srmConnectorIsConnected(connector))
                initConnector(connector);
        }
    }

    while (1)
        if (srmCoreProcessMonitor(core, -1) < 0)
            break;

    srmCoreDestroy(core);
    return 0;
}
