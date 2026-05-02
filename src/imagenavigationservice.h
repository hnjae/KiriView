// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGENAVIGATIONSERVICE_H
#define KIRIVIEW_IMAGENAVIGATIONSERVICE_H

#include "imagecandidaterepository.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <optional>
#include <vector>

namespace KiriView {
enum class ContainerNavigationError {
    Generic,
    EmptyContainer,
    InvalidComicBookArchive,
};

class ImageNavigationService final : public QObject
{
public:
    struct DisplayContext {
        bool hasDisplayedImage = false;
        QUrl displayedUrl;
        QUrl comicBookRootUrl;
    };

    using OpenUrlCallback = std::function<void(const QUrl &)>;
    using OpenContainerImageCallback = std::function<void(const QUrl &, const QUrl &)>;
    using ContainerNavigationErrorCallback
        = std::function<void(const QUrl &, ContainerNavigationError, const QString &)>;
    using PageNavigationChangedCallback = std::function<void()>;

    explicit ImageNavigationService(QObject *parent = nullptr);
    ImageNavigationService(QObject *parent, ImageNavigationCandidateProvider candidateProvider);

    void setOpenUrlCallback(OpenUrlCallback callback);
    void setOpenContainerImageCallback(OpenContainerImageCallback callback);
    void setContainerNavigationErrorCallback(ContainerNavigationErrorCallback callback);
    void setPageNavigationChangedCallback(PageNavigationChangedCallback callback);

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
    void finishContainerNavigationLoadWithRepositoryError(
        const QUrl &containerUrl, ImageCandidateRepositoryError error, const QString &errorString);

    void setPageNavigationUrls(std::vector<QUrl> urls, const QUrl &currentUrl);
    void setFallbackPageNavigationUrl(const QUrl &currentUrl);

    OpenUrlCallback m_openUrl;
    OpenContainerImageCallback m_openContainerImage;
    ContainerNavigationErrorCallback m_containerNavigationError;
    PageNavigationChangedCallback m_pageNavigationChanged;
    ImageCandidateRepository m_candidateRepository;
    ImageIoJob m_navigationListerJob;
    ImageIoJob m_containerNavigationListerJob;
    ImageIoJob m_containerNavigationListJob;
    ImageIoJob m_pageNavigationListerJob;
    std::vector<QUrl> m_pageNavigationUrls;
    int m_currentPageIndex = -1;
};
}

#endif
