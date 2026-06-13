// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportcommandstate.h"

namespace kiriview {
const ImageViewportProjection &ImageViewportCommandState::projection() const
{
    return m_projection;
}

QSizeF ImageViewportCommandState::viewportSize() const { return m_viewportSize; }

QSizeF ImageViewportCommandState::displaySize() const { return m_displaySize; }

bool ImageViewportCommandState::setGeometry(
    const QSizeF &viewportSize, const QSizeF &displaySize, ImageViewportObservationOrigin origin)
{
    if (m_viewportSize == viewportSize && m_displaySize == displaySize) {
        return false;
    }

    const bool initialized = m_geometryInitialized;
    m_viewportSize = viewportSize;
    m_displaySize = displaySize;
    m_geometryInitialized = true;
    if (initialized) {
        ++m_projection.observationRevision;
    }
    m_projection.observationOrigin = origin;
    m_projection.status = ImageViewportCommandStatus::Settled;
    return setFrame(project(m_projection.frame.contentPosition));
}

ImageViewportCommand ImageViewportCommandState::requestContentPosition(
    const QPointF &contentPosition)
{
    const ImageViewportFrame frame = project(contentPosition);
    ++m_projection.commandRevision;
    m_projection.requestedContentPosition = frame.contentPosition;
    m_projection.frame = frame;
    m_projection.status = ImageViewportCommandStatus::Pending;
    return ImageViewportCommand {
        m_projection.commandRevision,
        m_projection.requestedContentPosition,
        m_projection.frame,
    };
}

bool ImageViewportCommandState::acknowledgeCommand(quint64 commandRevision)
{
    if (commandRevision != m_projection.commandRevision) {
        m_projection.status = commandRevision < m_projection.commandRevision
            ? ImageViewportCommandStatus::Superseded
            : ImageViewportCommandStatus::Rejected;
        return false;
    }

    m_projection.appliedCommandRevision = commandRevision;
    ++m_projection.observationRevision;
    m_projection.observationOrigin = ImageViewportObservationOrigin::Command;
    m_projection.status = ImageViewportCommandStatus::Acknowledged;
    return true;
}

bool ImageViewportCommandState::acknowledgeCommand(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    return completeCommandApplication(commandRevision, actualContentPosition)
        && acknowledgeCommand(commandRevision);
}

bool ImageViewportCommandState::observeContentPosition(
    const QPointF &contentPosition, ImageViewportObservationOrigin origin)
{
    const ImageViewportFrame previousFrame = m_projection.frame;
    const ImageViewportCommandStatus previousStatus = m_projection.status;
    const QPointF previousRequestedContentPosition = m_projection.requestedContentPosition;
    const ImageViewportFrame frame = project(contentPosition);

    ++m_projection.observationRevision;
    m_projection.observationOrigin = origin;
    m_projection.status = ImageViewportCommandStatus::Settled;
    m_projection.frame = frame;
    m_projection.requestedContentPosition = frame.contentPosition;

    return previousFrame != m_projection.frame || previousStatus != m_projection.status
        || previousRequestedContentPosition != m_projection.requestedContentPosition;
}

bool ImageViewportCommandState::markCommandApplying(quint64 commandRevision)
{
    if (commandRevision != m_projection.commandRevision) {
        m_projection.status = commandRevision < m_projection.commandRevision
            ? ImageViewportCommandStatus::Superseded
            : ImageViewportCommandStatus::Rejected;
        return false;
    }

    m_projection.status = ImageViewportCommandStatus::Applying;
    return true;
}

bool ImageViewportCommandState::markCommandApplied(quint64 commandRevision)
{
    if (commandRevision != m_projection.commandRevision) {
        m_projection.status = commandRevision < m_projection.commandRevision
            ? ImageViewportCommandStatus::Superseded
            : ImageViewportCommandStatus::Rejected;
        return false;
    }

    m_projection.appliedCommandRevision = commandRevision;
    m_projection.status = ImageViewportCommandStatus::Applied;
    return true;
}

bool ImageViewportCommandState::completeCommandApplication(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    if (!markCommandApplied(commandRevision)) {
        return false;
    }

    const ImageViewportFrame frame = project(actualContentPosition);
    setFrame(frame);
    m_projection.requestedContentPosition = m_projection.frame.contentPosition;
    m_projection.observationOrigin = ImageViewportObservationOrigin::Command;
    m_projection.status = ImageViewportCommandStatus::Applied;
    return true;
}

ImageViewportFrame ImageViewportCommandState::project(const QPointF &contentPosition) const
{
    return projectImageViewportFrame(m_viewportSize, m_displaySize, contentPosition);
}

bool ImageViewportCommandState::setFrame(const ImageViewportFrame &frame)
{
    if (m_projection.frame == frame) {
        m_projection.requestedContentPosition = frame.contentPosition;
        return false;
    }

    m_projection.frame = frame;
    m_projection.requestedContentPosition = frame.contentPosition;
    return true;
}
}
