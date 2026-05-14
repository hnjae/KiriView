// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDELETIONEFFECTS_H
#define KIRIVIEW_IMAGEDELETIONEFFECTS_H

#include <QString>
#include <QUrl>
#include <utility>
#include <variant>

namespace KiriView {
struct ClearDeletedImageAfterDeletionEffect {
};

struct OpenImageDeletionFallbackEffect {
    QUrl url;
};

struct OpenContainerImageDeletionFallbackEffect {
    QUrl imageUrl;
    QUrl containerUrl;
};

struct ReportImageDeletionFailureEffect {
    QString errorString;
};

struct ImageDeletionEffect {
    using Payload
        = std::variant<ClearDeletedImageAfterDeletionEffect, OpenImageDeletionFallbackEffect,
            OpenContainerImageDeletionFallbackEffect, ReportImageDeletionFailureEffect>;

    static ImageDeletionEffect clearDeletedImage()
    {
        return ImageDeletionEffect(ClearDeletedImageAfterDeletionEffect {});
    }

    static ImageDeletionEffect openImageFallback(const QUrl &url)
    {
        return ImageDeletionEffect(OpenImageDeletionFallbackEffect { url });
    }

    static ImageDeletionEffect openContainerImageFallback(
        const QUrl &imageUrl, const QUrl &containerUrl)
    {
        return ImageDeletionEffect(
            OpenContainerImageDeletionFallbackEffect { imageUrl, containerUrl });
    }

    static ImageDeletionEffect reportFailure(const QString &errorString)
    {
        return ImageDeletionEffect(ReportImageDeletionFailureEffect { errorString });
    }

    explicit ImageDeletionEffect(Payload effectPayload)
        : payload(std::move(effectPayload))
    {
    }

    Payload payload;
};
}

#endif
