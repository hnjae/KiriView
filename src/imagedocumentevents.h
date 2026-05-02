// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEVENTS_H
#define KIRIVIEW_IMAGEDOCUMENTEVENTS_H

#include <QString>
#include <QUrl>

namespace KiriView {
enum class DocumentCommandType {
    ClearImage,
    UpdatePageNavigation,
    ScheduleAdjacentImagePredecode,
    OpenUrl,
    OpenContainerImage,
    FinishEmptyContainerNavigation,
    FinishContainerNavigationError,
    FinishAnimationError,
};

struct DocumentCommand {
    static DocumentCommand clearImage() { return DocumentCommand(DocumentCommandType::ClearImage); }

    static DocumentCommand updatePageNavigation()
    {
        return DocumentCommand(DocumentCommandType::UpdatePageNavigation);
    }

    static DocumentCommand scheduleAdjacentImagePredecode()
    {
        return DocumentCommand(DocumentCommandType::ScheduleAdjacentImagePredecode);
    }

    static DocumentCommand openUrl(const QUrl &url)
    {
        return DocumentCommand(DocumentCommandType::OpenUrl, url);
    }

    static DocumentCommand openContainerImage(const QUrl &imageUrl, const QUrl &containerUrl)
    {
        return DocumentCommand(DocumentCommandType::OpenContainerImage, imageUrl, containerUrl);
    }

    static DocumentCommand finishEmptyContainerNavigation(const QUrl &containerUrl)
    {
        return DocumentCommand(
            DocumentCommandType::FinishEmptyContainerNavigation, QUrl(), containerUrl);
    }

    static DocumentCommand finishContainerNavigationError(
        const QUrl &containerUrl, const QString &errorString)
    {
        return DocumentCommand(
            DocumentCommandType::FinishContainerNavigationError, QUrl(), containerUrl, errorString);
    }

    static DocumentCommand finishAnimationError(const QString &errorString)
    {
        return DocumentCommand(
            DocumentCommandType::FinishAnimationError, QUrl(), QUrl(), errorString);
    }

    explicit DocumentCommand(DocumentCommandType commandType, const QUrl &commandUrl = QUrl(),
        const QUrl &commandContainerUrl = QUrl(), const QString &commandErrorString = QString())
        : type(commandType)
        , url(commandUrl)
        , containerUrl(commandContainerUrl)
        , errorString(commandErrorString)
    {
    }

    DocumentCommandType type = DocumentCommandType::ClearImage;
    QUrl url;
    QUrl containerUrl;
    QString errorString;
};

enum class DocumentEventType {
    ClearImageRequested,
    PageNavigationUpdateRequested,
    AdjacentImagePredecodeRequested,
    OpenUrlRequested,
    ContainerImageSelected,
    EmptyContainerSelected,
    ContainerNavigationFailed,
    AnimationFailed,
};

struct DocumentEvent {
    static DocumentEvent clearImageRequested()
    {
        return DocumentEvent(DocumentEventType::ClearImageRequested);
    }

    static DocumentEvent pageNavigationUpdateRequested()
    {
        return DocumentEvent(DocumentEventType::PageNavigationUpdateRequested);
    }

    static DocumentEvent adjacentImagePredecodeRequested()
    {
        return DocumentEvent(DocumentEventType::AdjacentImagePredecodeRequested);
    }

    static DocumentEvent openUrlRequested(const QUrl &url)
    {
        return DocumentEvent(DocumentEventType::OpenUrlRequested, url);
    }

    static DocumentEvent containerImageSelected(const QUrl &imageUrl, const QUrl &containerUrl)
    {
        return DocumentEvent(DocumentEventType::ContainerImageSelected, imageUrl, containerUrl);
    }

    static DocumentEvent emptyContainerSelected(const QUrl &containerUrl)
    {
        return DocumentEvent(DocumentEventType::EmptyContainerSelected, QUrl(), containerUrl);
    }

    static DocumentEvent containerNavigationFailed(
        const QUrl &containerUrl, const QString &errorString)
    {
        return DocumentEvent(
            DocumentEventType::ContainerNavigationFailed, QUrl(), containerUrl, errorString);
    }

    static DocumentEvent animationFailed(const QString &errorString)
    {
        return DocumentEvent(DocumentEventType::AnimationFailed, QUrl(), QUrl(), errorString);
    }

    explicit DocumentEvent(DocumentEventType eventType, const QUrl &eventUrl = QUrl(),
        const QUrl &eventContainerUrl = QUrl(), const QString &eventErrorString = QString())
        : type(eventType)
        , url(eventUrl)
        , containerUrl(eventContainerUrl)
        , errorString(eventErrorString)
    {
    }

    DocumentEventType type = DocumentEventType::ClearImageRequested;
    QUrl url;
    QUrl containerUrl;
    QString errorString;
};
}

#endif
