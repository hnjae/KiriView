// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SESSION_MEDIAOPENWITH_H
#define KIRIVIEW_SESSION_MEDIAOPENWITH_H

#include "async/imageiojob.h"
#include "session/mediaopenwithplan.h"

#include <QString>
#include <functional>

class QObject;

namespace kiriview {
enum class MediaOpenWithResult {
    Succeeded,
    Canceled,
    Failed,
};

using MediaOpenWithCallback = std::function<void(MediaOpenWithResult, const QString &)>;
using MediaOpenWithProvider
    = std::function<ImageIoJob(QObject *, MediaOpenWithRequest, MediaOpenWithCallback)>;

MediaOpenWithProvider defaultMediaOpenWithProvider();
MediaOpenWithProvider mediaOpenWithProviderWithDefault(MediaOpenWithProvider provider);
}

#endif
