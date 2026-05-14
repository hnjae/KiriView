// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGENAVIGATIONSERVICE_H
#define KIRIVIEW_IMAGENAVIGATIONSERVICE_H

#include "imagecandidaterepository.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <optional>
#include <vector>

namespace KiriView {
using ContainerNavigationError = ImageCandidateRepositoryError;

class ImageNavigationService final : public QObject
{
public:
    struct DisplayContext {
        bool hasDisplayedImage = false;
        DisplayedImageLocation location;
    };

    using OpenUrlCallback = std::function<void(const QUrl &)>;
    using OpenContainerImageCallback = std::function<void(const QUrl &, const QUrl &)>;
    using ContainerNavigationErrorCallback
        = std::function<void(const QUrl &, ContainerNavigationError, const QString &)>;
    using PageNavigationChangedCallback = std::function<void()>;

    struct Callbacks {
        OpenUrlCallback openUrl;
        OpenContainerImageCallback openContainerImage;
        ContainerNavigationErrorCallback containerNavigationError;
        PageNavigationChangedCallback pageNavigationChanged;
    };

    ImageNavigationService(
        QObject *parent, ImageNavigationCandidateProvider candidateProvider, Callbacks callbacks);

    int currentPageNumber() const;
    int imageCount() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;

    void openAdjacentImage(const DisplayContext &context, NavigationDirection direction);
    void openAdjacentContainer(const QUrl &currentContainerUrl, NavigationDirection direction);
    void updatePageNavigation(const DisplayContext &context);

    void cancelNavigation();
    void cancelContainerNavigation();
    void cancelPageNavigationUpdate();
    void clearPageNavigation();

private:
    void finishNavigation(std::vector<ImageNavigationCandidate> candidates,
        NavigationDirection direction, const QUrl &currentUrl);

    void finishContainerNavigation(std::vector<ContainerNavigationCandidate> candidates,
        NavigationDirection direction, const QUrl &currentContainerUrl);
    void openImageFromContainerNavigation(const QUrl &imageUrl, const QUrl &containerUrl);
    void finishContainerNavigationLoadWithError(
        const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString);

    void setPageNavigationUrls(
        std::vector<QUrl> urls, const QUrl &currentUrl, ImageCandidateListSource source);
    void setPageNavigationState(PageNavigationState state);

    Callbacks m_callbacks;
    ImageCandidateRepository m_candidateRepository;
    ImageIoJob m_navigationListerJob;
    ImageIoJob m_containerNavigationListerJob;
    ImageIoJob m_containerNavigationListJob;
    ImageIoJob m_pageNavigationListerJob;
    PageNavigationState m_pageNavigation;
    std::optional<ImageCandidateListSource> m_pageNavigationSource;
};
}

#endif
