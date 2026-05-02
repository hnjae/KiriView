// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEVENTS_H
#define KIRIVIEW_IMAGEDOCUMENTEVENTS_H

#include <QString>
#include <QUrl>
#include <utility>
#include <variant>

namespace KiriView {
struct ClearImageRequestedEvent {
};

struct PageNavigationUpdateRequestedEvent {
};

struct AdjacentImagePredecodeRequestedEvent {
};

struct OpenUrlRequestedEvent {
    QUrl url;
};

struct ContainerImageSelectedEvent {
    QUrl imageUrl;
    QUrl containerUrl;
};

struct EmptyContainerSelectedEvent {
    QUrl containerUrl;
};

struct ContainerNavigationFailedEvent {
    QUrl containerUrl;
    QString errorString;
};

struct AnimationFailedEvent {
    QString errorString;
};

struct DocumentEvent {
    using Payload = std::variant<ClearImageRequestedEvent, PageNavigationUpdateRequestedEvent,
        AdjacentImagePredecodeRequestedEvent, OpenUrlRequestedEvent, ContainerImageSelectedEvent,
        EmptyContainerSelectedEvent, ContainerNavigationFailedEvent, AnimationFailedEvent>;

    static DocumentEvent clearImageRequested()
    {
        return DocumentEvent(ClearImageRequestedEvent {});
    }

    static DocumentEvent pageNavigationUpdateRequested()
    {
        return DocumentEvent(PageNavigationUpdateRequestedEvent {});
    }

    static DocumentEvent adjacentImagePredecodeRequested()
    {
        return DocumentEvent(AdjacentImagePredecodeRequestedEvent {});
    }

    static DocumentEvent openUrlRequested(const QUrl &url)
    {
        return DocumentEvent(OpenUrlRequestedEvent { url });
    }

    static DocumentEvent containerImageSelected(const QUrl &imageUrl, const QUrl &containerUrl)
    {
        return DocumentEvent(ContainerImageSelectedEvent { imageUrl, containerUrl });
    }

    static DocumentEvent emptyContainerSelected(const QUrl &containerUrl)
    {
        return DocumentEvent(EmptyContainerSelectedEvent { containerUrl });
    }

    static DocumentEvent containerNavigationFailed(
        const QUrl &containerUrl, const QString &errorString)
    {
        return DocumentEvent(ContainerNavigationFailedEvent { containerUrl, errorString });
    }

    static DocumentEvent animationFailed(const QString &errorString)
    {
        return DocumentEvent(AnimationFailedEvent { errorString });
    }

    explicit DocumentEvent(Payload eventPayload)
        : payload(std::move(eventPayload))
    {
    }

    Payload payload;
};
}

#endif
