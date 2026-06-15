// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONVIDEOOUTPUTRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONVIDEOOUTPUTRUNTIME_H

#include <QPointer>
#include <QString>
#include <QtGlobal>

class QObject;

namespace kiriview {
enum class DocumentSessionVideoOutputClaimAction { Reject, Attach, Detach };

struct DocumentSessionVideoOutputClaimReport {
    QString claimToken;
    QObject *surfaceOwner = nullptr;
    bool attachRequested = false;
};

class DocumentSessionVideoOutputRuntime final
{
public:
    QString nextSurfaceClaimToken();
    DocumentSessionVideoOutputClaimAction reportSurfaceClaim(
        const DocumentSessionVideoOutputClaimReport &report);
    void clear();

private:
    QPointer<QObject> m_surfaceClaimOwner;
    quint64 m_nextSurfaceClaimRevision = 0;
    quint64 m_surfaceClaimRevision = 0;
};
}

#endif
