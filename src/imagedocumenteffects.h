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

struct PrepareFailedContainerEffect {
    QUrl containerUrl;
};

struct AnimationFailedEffect {
    QString errorString;
};

struct ImageDocumentEffect {
    using Payload = std::variant<ClearImageEffect, ResetZoomEffect, UpdatePageNavigationEffect,
        ScheduleAdjacentImagePredecodeEffect, OpenUrlEffect, ContainerImageSelectedEffect,
        EmptyContainerSelectedEffect, ContainerNavigationFailedEffect, PrepareFailedContainerEffect,
        AnimationFailedEffect>;

    static ImageDocumentEffect clearImage() { return ImageDocumentEffect(ClearImageEffect {}); }

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

    static ImageDocumentEffect prepareFailedContainer(const QUrl &containerUrl)
    {
        return ImageDocumentEffect(PrepareFailedContainerEffect { containerUrl });
    }

    static ImageDocumentEffect animationFailed(const QString &errorString)
    {
        return ImageDocumentEffect(AnimationFailedEffect { errorString });
    }

    explicit ImageDocumentEffect(Payload effectPayload)
        : payload(std::move(effectPayload))
    {
    }

    Payload payload;
};

class ImageDocumentEffects
{
public:
    void add(ImageDocumentEffect effect) { m_effects.push_back(std::move(effect)); }

    bool empty() const { return m_effects.empty(); }

    const std::vector<ImageDocumentEffect> &items() const { return m_effects; }

private:
    std::vector<ImageDocumentEffect> m_effects;
};
}

#endif
