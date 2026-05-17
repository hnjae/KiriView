// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEFFECTS_H
#define KIRIVIEW_IMAGEDOCUMENTEFFECTS_H

#include <QString>
#include <QUrl>
#include <utility>
#include <variant>
#include <vector>

namespace KiriView {
struct ClearImageEffect {
};

struct ClearDeletedImageEffect {
};

struct ResetZoomEffect {
};

struct UpdatePageNavigationEffect {
};

struct ScheduleAdjacentImagePredecodeEffect {
};

struct OpenUrlEffect {
    QUrl url;
};

struct ContainerImageSelectedEffect {
    QUrl imageUrl;
    QUrl containerUrl;
};

struct EmptyContainerSelectedEffect {
    QUrl containerUrl;
};

struct ContainerNavigationFailedEffect {
    QUrl containerUrl;
    QString errorString;
};

struct PageNavigationSelectedEffect {
    QUrl url;
    bool preserveTwoPageSpreadTransition = false;
};

struct PrepareFailedContainerEffect {
    QUrl containerUrl;
};

struct ImageDocumentEffect {
    using Payload = std::variant<ClearImageEffect, ClearDeletedImageEffect, ResetZoomEffect,
        UpdatePageNavigationEffect, ScheduleAdjacentImagePredecodeEffect, OpenUrlEffect,
        ContainerImageSelectedEffect, EmptyContainerSelectedEffect, ContainerNavigationFailedEffect,
        PageNavigationSelectedEffect, PrepareFailedContainerEffect>;

    static ImageDocumentEffect clearImage() { return ImageDocumentEffect(ClearImageEffect {}); }

    static ImageDocumentEffect clearDeletedImage()
    {
        return ImageDocumentEffect(ClearDeletedImageEffect {});
    }

    static ImageDocumentEffect resetZoom() { return ImageDocumentEffect(ResetZoomEffect {}); }

    static ImageDocumentEffect updatePageNavigation()
    {
        return ImageDocumentEffect(UpdatePageNavigationEffect {});
    }

    static ImageDocumentEffect scheduleAdjacentImagePredecode()
    {
        return ImageDocumentEffect(ScheduleAdjacentImagePredecodeEffect {});
    }

    static ImageDocumentEffect openUrl(const QUrl &url)
    {
        return ImageDocumentEffect(OpenUrlEffect { url });
    }

    static ImageDocumentEffect containerImageSelected(
        const QUrl &imageUrl, const QUrl &containerUrl)
    {
        return ImageDocumentEffect(ContainerImageSelectedEffect { imageUrl, containerUrl });
    }

    static ImageDocumentEffect emptyContainerSelected(const QUrl &containerUrl)
    {
        return ImageDocumentEffect(EmptyContainerSelectedEffect { containerUrl });
    }

    static ImageDocumentEffect containerNavigationFailed(
        const QUrl &containerUrl, const QString &errorString)
    {
        return ImageDocumentEffect(ContainerNavigationFailedEffect { containerUrl, errorString });
    }

    static ImageDocumentEffect pageNavigationSelected(
        const QUrl &url, bool preserveTwoPageSpreadTransition)
    {
        return ImageDocumentEffect(
            PageNavigationSelectedEffect { url, preserveTwoPageSpreadTransition });
    }

    static ImageDocumentEffect prepareFailedContainer(const QUrl &containerUrl)
    {
        return ImageDocumentEffect(PrepareFailedContainerEffect { containerUrl });
    }

    explicit ImageDocumentEffect(Payload effectPayload)
        : payload(std::move(effectPayload))
    {
    }

    Payload payload;
};

using ImageDocumentEffects = std::vector<ImageDocumentEffect>;
}

#endif
