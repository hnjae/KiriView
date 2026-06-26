// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONVIDEOOUTPUTRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONVIDEOOUTPUTRUNTIME_H

#include <QPointer>
#include <QRectF>
#include <QString>
#include <QtGlobal>
#include <functional>

class QObject;

namespace kiriview {
struct DocumentSessionVideoOutputAttachmentPort
{
    std::function<void(QObject*)> setVideoOutput;
    std::function<void(const QRectF&, const QRectF&)> setVideoOutputGeometry;
};

struct DocumentSessionVideoOutputClaimReport
{
    QString claimToken;
    QObject* surfaceOwner = nullptr;
    QObject* videoOutput = nullptr;
    bool attachRequested = false;
    QRectF contentRect;
    QRectF sourceRect;
};

class DocumentSessionVideoOutputRuntime final
{
public:
    QString nextSurfaceClaimToken();
    bool reportSurfaceClaim(const DocumentSessionVideoOutputClaimReport& report,
        const DocumentSessionVideoOutputAttachmentPort& attachmentPort);
    void clearAttachment(const DocumentSessionVideoOutputAttachmentPort& attachmentPort);
    void clear();

private:
    QPointer<QObject> m_surfaceClaimOwner;
    quint64 m_nextSurfaceClaimRevision = 0;
    quint64 m_surfaceClaimRevision = 0;
};
}

#endif
