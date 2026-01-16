#include "PipewireGrabber.hpp"
#include "proxyPipewire/include/smartPipewire.h"

#include <dlfcn.h>

bool (*_hasPipewire)() = nullptr;
const char* (*_getPipewireError)() = nullptr;
void (*_initPipewireDisplay)(const char* restorationToken) = nullptr;
void (*_uninitPipewireDisplay)() = nullptr;
PipewireImage (*_getFramePipewire)() = nullptr;
void (*_releaseFramePipewire)() = nullptr;
const char* (*_getPipewireToken)() = nullptr;

struct GrabberData
{
    GrabberData()
    {
    }
};

PipewireGrabber::PipewireGrabber(QObject *parent, GrabberContext * gcontext)
    : GrabberBase(parent, gcontext), _library(nullptr), m_semaphore(1), m_inited(false)
{
    _library = dlopen("libproxyPipewire.so", RTLD_NOW);

    if (_library)
    {
        _getPipewireToken = (const char* (*)()) dlsym(_library, "getPipewireToken");
        _getPipewireError = (const char* (*)()) dlsym(_library, "getPipewireError");
        _hasPipewire = (bool (*)()) dlsym(_library, "hasPipewire");
        _initPipewireDisplay = (void (*)(const char*)) dlsym(_library, "initPipewireDisplay");
        _uninitPipewireDisplay = (void (*)()) dlsym(_library, "uniniPipewireDisplay");
        _getFramePipewire = (PipewireImage (*)()) dlsym(_library, "getFramePipewire");
        _releaseFramePipewire = (void (*)()) dlsym(_library, "releaseFramePipewire");
    }
    else
        qCritical() << Q_FUNC_INFO << "Could not load Pipewire proxy library. Error: " << dlerror();

    if (_library && (_getPipewireToken == nullptr || _hasPipewire == nullptr || _releaseFramePipewire == nullptr || _initPipewireDisplay == nullptr || _uninitPipewireDisplay == nullptr || _getFramePipewire == nullptr))
    {
        qCritical() << Q_FUNC_INFO << "Could not load Pipewire proxy library definition. Error: " << dlerror();

        dlclose(_library);
        _library = nullptr;
    }
}

void PipewireGrabber::startGrabbing()
{
    qCritical() << Q_FUNC_INFO << "start grabbing";
    m_semaphore.acquire();
    if (_initPipewireDisplay && _hasPipewire())
    {
        DEBUG_HIGH_LEVEL << Q_FUNC_INFO << "Token:" << m_token;
        _initPipewireDisplay(m_token.toLatin1().constData());
        m_inited = true;
        GrabberBase::startGrabbing();
    }
    m_semaphore.release();
}

void PipewireGrabber::stopGrabbing()
{
    GrabberBase::stopGrabbing();
    if (_library)
    {
        m_semaphore.acquire();
        m_token = QString("%1").arg(_getPipewireToken());
        if (_uninitPipewireDisplay)
            _uninitPipewireDisplay();
        m_semaphore.release();
    }
}

QList<ScreenInfo> * PipewireGrabber::screensWithWidgets(QList<ScreenInfo> *result, const QList<GrabWidget *> &grabWidgets)
{
    result->clear();
    if (!_library)
        return result;

    if (m_semaphore.tryAcquire())
    {
        PipewireImage img = _getFramePipewire();
        DEBUG_HIGH_LEVEL << Q_FUNC_INFO << "screensWithWidgets " << img.width << "x" << img.height;

        for (int i = 0; i < 1; ++i) {
            ScreenInfo screen;
            intptr_t handle = i;
            screen.handle = reinterpret_cast<void *>(handle);
            screen.rect = QRect(0, 0, img.width, img.height);
            for (int k = 0; k < grabWidgets.size(); ++k) {
                if (screen.rect.intersects(grabWidgets[k]->rect())) {
                    result->append(screen);
                    break;
                }
            }
            break; //FIXME only one
        }
        m_semaphore.release();
    }

    return result;
}

bool PipewireGrabber::reallocate(const QList<ScreenInfo> &screens)
{
    freeScreens();
    if (!_library)
        return false;

    qCritical() << Q_FUNC_INFO << "reallocate";
    // There's probably ever going to be only a single "screen".
    // From the portal you choose a single monitor or a whole workspace.
    PipewireImage img = _getFramePipewire();
    const int pitch = img.width * 4;
    DEBUG_HIGH_LEVEL << "dimensions " << img.width << "x" << img.height << "is rgb:" << img.isOrderRgb;
    //qCritical() << "dimensions " << img.width << "x" << img.height << "is rgb:" << img.isOrderRgb;

    int imagesize = img.height * pitch;

    GrabbedScreen grabScreen;
    m_buffers.append(new unsigned char[imagesize]);
    grabScreen.imgData = m_buffers[0];
    grabScreen.imgDataSize = imagesize;
    grabScreen.bytesPerRow = pitch;
    grabScreen.imgFormat = img.isOrderRgb ? BufferFormatAbgr : BufferFormatArgb; // TODO video format BGRA, but need to parse as ARGB?
    grabScreen.screenInfo = screens[0];
    grabScreen.associatedData = nullptr;
    _screensWithWidgets.append(grabScreen);

    return true;
}

GrabResult PipewireGrabber::grabScreens()
{
    const int stride = 4;

    if (!_library)
        return GrabResultError;

    if (m_semaphore.tryAcquire())
    {
        PipewireImage img = _getFramePipewire();
        for (int i = 0; i < _screensWithWidgets.size(); ++i) {
            if (!img.data)
            {
                if (img.isError)
                    return GrabResultError;
            }
            else
            {
                DEBUG_HIGH_LEVEL << "grabScreens: " << img.width << "x" << img.height;
                memcpy(m_buffers[i], img.data, img.width * img.height * stride);
            }
        }
        m_semaphore.release();
    }

    return GrabResultOk;
}

void PipewireGrabber::freeScreens()
{
    for (int i = 0; i < _screensWithWidgets.size(); ++i) {
        GrabberData *d = reinterpret_cast<GrabberData *>(_screensWithWidgets[i].associatedData);
        delete d;
        _screensWithWidgets[i].associatedData = nullptr;
    }

    _screensWithWidgets.clear();
    m_buffers.clear();

}

PipewireGrabber::~PipewireGrabber()
{
    freeScreens();
    if (_library)
    {
        _uninitPipewireDisplay();
        dlclose(_library);
        _library = nullptr;
    }
}
