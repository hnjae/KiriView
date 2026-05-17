// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENTRANSITIONAPPLIER_H
#define KIRIVIEW_IMAGEOPENTRANSITIONAPPLIER_H

#include "imagedocumenteffects.h"
#include "imageloadtypes.h"
#include "kiriview/src/imageopenworkflow.cxx.h"

#include <QString>
#include <QUrl>
#include <optional>

namespace KiriView {
class ImageDocumentState;

struct ImageOpenTransitionContext {
    const ImageLoadSession *session = nullptr;
    std::optional<QUrl> containerUrl;
    std::optional<QUrl> displayedUrl;
    std::optional<QString> errorString;

    static ImageOpenTransitionContext successfulImageLoad(const ImageLoadSession &session);
    static ImageOpenTransitionContext sourceLoadError(
        const ImageLoadSession &session, const QUrl &displayedUrl, const QString &errorString);
    static ImageOpenTransitionContext containerNavigationError(
        const QUrl &containerUrl, const QString &errorString);
    static ImageOpenTransitionContext animationError(const QString &errorString);

    QUrl urlForTarget(ImageOpenUrlTarget target) const;
    const DisplayedImageLocation *sessionLocation() const;
    QString providedErrorString() const;
};

ImageDocumentEffects applyImageOpenTransition(ImageDocumentState &state,
    const ImageOpenTransition &transition, const ImageOpenTransitionContext &context = {});
}

#endif
