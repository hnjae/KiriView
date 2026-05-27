// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAOPENWITH_H
#define KIRIVIEW_MEDIAOPENWITH_H

#include "async/imageiojob.h"

#include <QString>
#include <QUrl>
#include <functional>

class QObject;

namespace KiriView {
enum class MediaOpenWithResult {
    Succeeded,
    Canceled,
    Failed,
};

struct MediaOpenWithRequest {
    QUrl targetUrl;
};

using MediaOpenWithCallback = std::function<void(MediaOpenWithResult, const QString &)>;
using MediaOpenWithProvider
    = std::function<ImageIoJob(QObject *, MediaOpenWithRequest, MediaOpenWithCallback)>;

MediaOpenWithProvider defaultMediaOpenWithProvider();
MediaOpenWithProvider mediaOpenWithProviderWithDefault(MediaOpenWithProvider provider);
}

#endif
