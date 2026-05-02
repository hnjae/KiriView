// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEVENTS_H
#define KIRIVIEW_IMAGEDOCUMENTEVENTS_H

#include <QString>
#include <QUrl>
#include <utility>
#include <variant>

namespace KiriView {
struct ClearImageCommand {
};

struct UpdatePageNavigationCommand {
};

struct ScheduleAdjacentImagePredecodeCommand {
};

struct OpenUrlCommand {
    QUrl url;
};

struct OpenContainerImageCommand {
    QUrl imageUrl;
    QUrl containerUrl;
};

struct FinishEmptyContainerNavigationCommand {
    QUrl containerUrl;
};

struct FinishContainerNavigationErrorCommand {
    QUrl containerUrl;
    QString errorString;
};

struct FinishAnimationErrorCommand {
    QString errorString;
};

struct DocumentCommand {
    using Payload = std::variant<ClearImageCommand, UpdatePageNavigationCommand,
        ScheduleAdjacentImagePredecodeCommand, OpenUrlCommand, OpenContainerImageCommand,
        FinishEmptyContainerNavigationCommand, FinishContainerNavigationErrorCommand,
        FinishAnimationErrorCommand>;

    static DocumentCommand clearImage() { return DocumentCommand(ClearImageCommand {}); }

    static DocumentCommand updatePageNavigation()
    {
        return DocumentCommand(UpdatePageNavigationCommand {});
    }

    static DocumentCommand scheduleAdjacentImagePredecode()
    {
        return DocumentCommand(ScheduleAdjacentImagePredecodeCommand {});
    }

    static DocumentCommand openUrl(const QUrl &url)
    {
        return DocumentCommand(OpenUrlCommand { url });
    }

    static DocumentCommand openContainerImage(const QUrl &imageUrl, const QUrl &containerUrl)
    {
        return DocumentCommand(OpenContainerImageCommand { imageUrl, containerUrl });
    }

    static DocumentCommand finishEmptyContainerNavigation(const QUrl &containerUrl)
    {
        return DocumentCommand(FinishEmptyContainerNavigationCommand { containerUrl });
    }

    static DocumentCommand finishContainerNavigationError(
        const QUrl &containerUrl, const QString &errorString)
    {
        return DocumentCommand(FinishContainerNavigationErrorCommand { containerUrl, errorString });
    }

    static DocumentCommand finishAnimationError(const QString &errorString)
    {
        return DocumentCommand(FinishAnimationErrorCommand { errorString });
    }

    explicit DocumentCommand(Payload commandPayload)
        : payload(std::move(commandPayload))
    {
    }

    Payload payload;
};

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
