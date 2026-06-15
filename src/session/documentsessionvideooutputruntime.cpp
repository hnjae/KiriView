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

DocumentSessionVideoOutputClaimAction DocumentSessionVideoOutputRuntime::reportSurfaceClaim(
    const DocumentSessionVideoOutputClaimReport &report)
{
    const std::optional<quint64> claimRevision = surfaceClaimRevisionFromToken(report.claimToken);
    if (!claimRevision || report.surfaceOwner == nullptr) {
        return DocumentSessionVideoOutputClaimAction::Reject;
    }

    const bool sameOwner = m_surfaceClaimOwner == report.surfaceOwner;
    if (sameOwner && *claimRevision < m_surfaceClaimRevision) {
        return DocumentSessionVideoOutputClaimAction::Reject;
    }

    if (!report.attachRequested) {
        if (!sameOwner) {
            return DocumentSessionVideoOutputClaimAction::Reject;
        }
        clear();
        return DocumentSessionVideoOutputClaimAction::Detach;
    }

    m_surfaceClaimOwner = report.surfaceOwner;
    m_surfaceClaimRevision = *claimRevision;
    return DocumentSessionVideoOutputClaimAction::Attach;
}

void DocumentSessionVideoOutputRuntime::clear()
{
    m_surfaceClaimOwner.clear();
    m_surfaceClaimRevision = 0;
}
}
