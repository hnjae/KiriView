// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_FILEDELETION_H
#define KIRIVIEW_FILEDELETION_H

#include "async/imageiojob.h"

#include <QString>
#include <QUrl>
#include <functional>

class QObject;

namespace KiriView {
enum class FileDeletionMode {
    MoveToTrash,
    DeletePermanently,
};

enum class FileDeletionResult {
    Succeeded,
    Canceled,
    Failed,
};

enum class FileDeletionCompletionAction {
    ClearDeletedTargetAndOpenFallback,
    Ignore,
    ReportFailure,
};

struct FileDeletionRequest {
    QUrl targetUrl;
    FileDeletionMode mode = FileDeletionMode::MoveToTrash;
};

using FileDeletionCallback = std::function<void(FileDeletionResult, const QString &)>;
using FileDeletionProvider
    = std::function<ImageIoJob(QObject *, FileDeletionRequest, FileDeletionCallback)>;

FileDeletionCompletionAction fileDeletionCompletionAction(FileDeletionResult result);
FileDeletionProvider defaultFileDeletionProvider();
FileDeletionProvider fileDeletionProviderWithDefault(FileDeletionProvider provider);
}

#endif
