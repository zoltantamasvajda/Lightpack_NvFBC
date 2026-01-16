/*
 * PipewireGrabber.hpp
 *
 *  Created on: 14.10.23
 *     Project: Lightpack
 *
 *  Copyright (c) 2011 Andrey Isupov, Timur Sattarov, Mike Shatohin
 *
 *  Lightpack a USB content-driving ambient lighting system
 *
 *  Lightpack is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Lightpack is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "GrabberBase.hpp"
#include "../src/enums.hpp"
#include "../src/debug.h"

#include <QSemaphore>

#ifdef X11_GRAB_SUPPORT

using namespace Grab;

class PipewireGrabber : public GrabberBase
{
public:
    PipewireGrabber(QObject *parent, GrabberContext *context);
    virtual ~PipewireGrabber();

    DECLARE_GRABBER_NAME("PipewireGrabber")

protected:
    virtual GrabResult grabScreens();
    virtual bool reallocate(const QList<ScreenInfo> &screens);
    virtual QList<ScreenInfo> * screensWithWidgets(QList<ScreenInfo> *result, const QList<GrabWidget *> &grabWidgets);
    virtual void startGrabbing();
    virtual void stopGrabbing();

private:
    void freeScreens();

private:
    void *_library;
    QString m_token;
    QSemaphore m_semaphore;
    bool m_inited;
    QVector<unsigned char*> m_buffers;
};
#endif // X11_GRAB_SUPPORT
