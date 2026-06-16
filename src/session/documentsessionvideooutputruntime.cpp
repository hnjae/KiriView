// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionvideooutputruntime.h"

#include <optional>

namespace {
QString surfaceClaimToken(quint64 revision) { return QString::number(revision); }

std::optional<quint64> surfaceClaimRevisionFromToken(const QString &token)
{
    bool ok = false;
    const quint64 revision = token.toULongLong(&ok);
    if (!ok) {
        return std::nullopt;
    }

    return revision;
}
}

namespace kiriview {
QString DocumentSessionVideoOutputRuntime::nextSurfaceClaimToken()
{
    ++m_nextSurfaceClaimRevision;
    return surfaceClaimToken(m_nextSurfaceClaimRevision);
}

bool DocumentSessionVideoOutputRuntime::reportSurfaceClaim(
    const DocumentSessionVideoOutputClaimReport &report,
    const DocumentSessionVideoOutputAttachmentPort &attachmentPort)
{
    const std::optional<quint64> claimRevision = surfaceClaimRevisionFromToken(report.claimToken);
    if (!claimRevision || report.surfaceOwner == nullptr) {
        return false;
    }

    const bool sameOwner = m_surfaceClaimOwner == report.surfaceOwner;
    if (sameOwner && *claimRevision < m_surfaceClaimRevision) {
        return false;
    }

    if (!report.attachRequested) {
        if (!sameOwner) {
            return false;
        }
        clearAttachment(attachmentPort);
        return true;
    }

    if (report.videoOutput == nullptr) {
        return false;
    }

    m_surfaceClaimOwner = report.surfaceOwner;
    m_surfaceClaimRevision = *claimRevision;
    if (attachmentPort.setVideoOutput) {
        attachmentPort.setVideoOutput(report.videoOutput);
    }
    if (attachmentPort.setVideoOutputGeometry) {
        attachmentPort.setVideoOutputGeometry(report.contentRect, report.sourceRect);
    }
    return true;
}

void DocumentSessionVideoOutputRuntime::clearAttachment(
    const DocumentSessionVideoOutputAttachmentPort &attachmentPort)
{
    clear();
    if (attachmentPort.setVideoOutput) {
        attachmentPort.setVideoOutput(nullptr);
    }
}

void DocumentSessionVideoOutputRuntime::clear()
{
    m_surfaceClaimOwner.clear();
    m_surfaceClaimRevision = 0;
}
}
