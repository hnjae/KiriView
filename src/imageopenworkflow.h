// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOW_H
#define KIRIVIEW_IMAGEOPENWORKFLOW_H

#include "imageloader.h"

#include <QString>
#include <QUrl>

namespace KiriView {
class ImageDocumentState;

struct ImageOpenCommands {
    bool clearImage = false;
    bool resetZoom = false;
    bool updatePageNavigation = false;
    bool scheduleAdjacentPredecode = false;
    QUrl failedContainerUrl;
};

class ImageOpenWorkflow
{
public:
    static ImageOpenCommands beginSourceLoad(ImageDocumentState &state, bool hasImage);
    static ImageOpenCommands finishEmptySourceLoad(ImageDocumentState &state);
    static ImageOpenCommands finishSuccessfulImageLoad(
        ImageDocumentState &state, const ImageLoadSession &session);
    static ImageOpenCommands finishContainerNavigationLoadWithError(
        ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString);
    static ImageOpenCommands finishReplacementLoadWithError(
        ImageDocumentState &state, const QString &errorString);
    static ImageOpenCommands finishInitialLoadWithError(
        ImageDocumentState &state, const QString &errorString);
};
}

#endif
