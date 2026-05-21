// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGENAVIGATIONSERVICE_H
#define KIRIVIEW_IMAGENAVIGATIONSERVICE_H

#include "imagecandidatelistsource.h"
#include "imagecandidaterepository.h"
#include "imagecontainernavigationcontroller.h"
#include "imagenavigationtypes.h"
#include "imagepagenavigationcontroller.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <optional>

namespace KiriView {
class ImageNavigationService final : public QObject
{
public:
    using OpenUrlCallback = std::function<void(const QUrl &)>;
    using OpenContainerImageCallback = std::function<void(const QUrl &, const QUrl &)>;
    using ContainerNavigationErrorCallback
        = std::function<void(const QUrl &, ContainerNavigationError, const QString &)>;
    using PageNavigationChangedCallback = std::function<void()>;
    using ClearCurrentImageCallback = std::function<void()>;
    using DeletionInProgressCallback = std::function<bool()>;

    struct Callbacks {
        OpenUrlCallback openUrl;
        OpenContainerImageCallback openContainerImage;
        ContainerNavigationErrorCallback containerNavigationError;
        PageNavigationChangedCallback pageNavigationChanged;
        ClearCurrentImageCallback clearCurrentImage;
        DeletionInProgressCallback deletionInProgress;
    };

    ImageNavigationService(
        QObject *parent, ImageNavigationCandidateProvider candidateProvider, Callbacks callbacks);

    int currentPageNumber() const;
    int imageCount() const;
    ImagePageNavigationSnapshot pageNavigationSnapshot() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;
    std::optional<QUrl> selectPage(int pageNumber);

    void openAdjacentImage(
        std::optional<ImageCandidateListContext> context, NavigationDirection direction);
    void openAdjacentContainer(const QUrl &currentContainerUrl, NavigationDirection direction);
    void updatePageNavigation(std::optional<ImageCandidateListContext> context);

    void cancelNavigation();
    void cancelContainerNavigation();
    void cancelPageNavigationUpdate();
    void cancelAllNavigation();
    void clearPageNavigation();

private:
    Callbacks m_callbacks;
    ImageCandidateRepository m_candidateRepository;
    ImageContainerNavigationController m_containerNavigation;
    ImagePageNavigationController m_pageNavigation;
};
}

#endif
