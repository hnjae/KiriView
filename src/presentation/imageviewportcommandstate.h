// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEVIEWPORTCOMMANDSTATE_H
#define KIRIVIEW_IMAGEVIEWPORTCOMMANDSTATE_H

#include "presentation/imageviewportframe.h"

#include <QPointF>
#include <QSizeF>
#include <QtGlobal>

namespace kiriview {
enum class ImageViewportCommandStatus {
    Pending,
    Applying,
    Applied,
    Acknowledged,
    Settled,
    Superseded,
    Rejected,
};

enum class ImageViewportObservationOrigin {
    Command,
    User,
    Inertia,
    Overshoot,
    Resize,
    Rotation,
    DevicePixelRatio,
    System,
};

struct ImageViewportCommand
{
    quint64 revision = 0;
    QPointF requestedContentPosition;
    ImageViewportFrame frame;
};

struct ImageViewportProjection
{
    quint64 commandRevision = 0;
    quint64 appliedCommandRevision = 0;
    quint64 observationRevision = 0;
    QPointF requestedContentPosition;
    ImageViewportFrame frame;
    ImageViewportCommandStatus status = ImageViewportCommandStatus::Settled;
    ImageViewportObservationOrigin observationOrigin = ImageViewportObservationOrigin::System;
};

class ImageViewportCommandState final
{
public:
    const ImageViewportProjection& projection() const;
    QSizeF viewportSize() const;
    QSizeF displaySize() const;

    bool setGeometry(QSizeF viewportSize, QSizeF displaySize,
        ImageViewportObservationOrigin origin = ImageViewportObservationOrigin::Resize);
    ImageViewportCommand requestContentPosition(QPointF contentPosition);
    bool acknowledgeCommand(quint64 commandRevision);
    bool acknowledgeCommand(quint64 commandRevision, QPointF actualContentPosition);
    bool observeContentPosition(QPointF contentPosition, ImageViewportObservationOrigin origin);
    bool markCommandApplying(quint64 commandRevision);
    bool markCommandApplied(quint64 commandRevision);
    bool completeCommandApplication(quint64 commandRevision, QPointF actualContentPosition);

private:
    ImageViewportFrame project(QPointF contentPosition) const;
    bool setFrame(const ImageViewportFrame& frame);

    QSizeF m_viewportSize;
    QSizeF m_displaySize;
    ImageViewportProjection m_projection;
    bool m_geometryInitialized = false;
};
}

#endif
