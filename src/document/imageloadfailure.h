// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADFAILURE_H
#define KIRIVIEW_IMAGELOADFAILURE_H

#include <QString>
#include <QUrl>
#include <QtGlobal>

namespace kiriview {
enum class ImageLoadFailureKind {
    DataLoad,
    Decode,
    OpenedCollectionLoad,
    EmptyOpenedCollection,
    Presentation,
};

enum class ImageLoadFailureSeverity {
    Error,
};

struct ImageLoadFailure
{
    QUrl sourceUrl;
    quint64 sessionId = 0;
    ImageLoadFailureKind kind = ImageLoadFailureKind::DataLoad;
    QString userMessage;
    QString diagnosticDetail;
    ImageLoadFailureSeverity severity = ImageLoadFailureSeverity::Error;
    bool retryable = false;
};
}

#endif
