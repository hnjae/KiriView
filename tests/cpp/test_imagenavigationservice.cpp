// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imagecallback.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagenavigationservice.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace {
using KiriView::ContainerNavigationCandidateType;
using KiriView::ImageNavigationCandidate;
using KiriView::NavigationDirection;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::containerCandidate;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

KiriView::ImageNavigationService::Callbacks navigationCallbacks(
    std::function<void(const QUrl &)> openUrl = {},
    std::function<void(const QUrl &, const QUrl &)> openContainerImage = {},
    std::function<void(const QUrl &, KiriView::ContainerNavigationError, const QString &)>
        containerNavigationError
    = {},
    KiriView::ImageNavigationService::PageNavigationChangedCallback pageNavigationChanged = {},
    std::function<void()> clearCurrentImage = {},
    KiriView::ImageNavigationService::DeletionInProgressCallback deletionInProgress = {})
{
    return KiriView::ImageNavigationService::Callbacks {
        [openUrl = std::move(openUrl), openContainerImage = std::move(openContainerImage),
            containerNavigationError = std::move(containerNavigationError),
            clearCurrentImage = std::move(clearCurrentImage)](
            KiriView::ImageNavigationPlan plan) mutable {
            for (const KiriView::ImageNavigationEffect &effect : plan) {
                if (const auto *openEffect
                    = std::get_if<KiriView::OpenImageNavigationUrlEffect>(&effect)) {
                    KiriView::invokeIfSet(openUrl, openEffect->target.url);
                } else if (const auto *containerEffect
                    = std::get_if<KiriView::OpenContainerImageNavigationEffect>(&effect)) {
                    KiriView::invokeIfSet(openContainerImage, containerEffect->target.url,
                        containerEffect->containerUrl);
                } else if (const auto *errorEffect
                    = std::get_if<KiriView::ReportContainerNavigationErrorEffect>(&effect)) {
                    KiriView::invokeIfSet(containerNavigationError, errorEffect->containerUrl,
                        errorEffect->error, errorEffect->errorString);
                } else if (std::holds_alternative<KiriView::ClearCurrentImageNavigationEffect>(
                               effect)) {
                    KiriView::invokeIfSet(clearCurrentImage);
                }
            }
        },
        std::move(pageNavigationChanged), std::move(deletionInProgress)
    };
}

std::optional<KiriView::ImageCandidateListContext> navigationContext(
    KiriView::DisplayedImageLocation displayedImage)
{
    return KiriView::imageCandidateListContextForDisplayedImage(std::move(displayedImage));
}

struct ManualImageCandidateList {
    QObject *object = nullptr;
    KiriView::ImagePageScopeLocation imagePageScope;
    KiriView::ImageCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualArchiveImageCandidateProvider
{
public:
    KiriView::ImageIoJob start(QObject *receiver, KiriView::ImagePageScopeLocation archiveDocument,
        KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualImageCandidateList>();
        load->imagePageScope = std::move(archiveDocument);
        load->callback = std::move(callback);
        load->errorCallback = std::move(errorCallback);

        KiriView::ImageIoJob job = KiriView::TestSupport::Detail::startManualIoJob(receiver, load);
        m_loads.push_back(load);
        return job;
    }

    std::size_t loadCount() const { return m_loads.size(); }

    ManualImageCandidateList &backLoad() { return *m_loads.back(); }

    void finishBackLoad(std::vector<ImageNavigationCandidate> candidates)
    {
        KiriView::TestSupport::Detail::finishManualIoJob(m_loads.back(),
            [candidates = std::move(candidates)](ManualImageCandidateList &load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    KiriView::ImageNavigationCandidateProvider provider()
    {
        return KiriView::ImageNavigationCandidateProvider {
            [](QObject *, QUrl, KiriView::ImageCandidatesCallback,
                KiriView::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected directory image listing"));
                }
                return KiriView::ImageIoJob();
            },
            [](QObject *, QUrl, KiriView::ContainerCandidatesCallback,
                KiriView::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected container listing"));
                }
                return KiriView::ImageIoJob();
            },
            [this](QObject *receiver, KiriView::ImagePageScopeLocation archiveDocument,
                KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback) {
                return start(receiver, std::move(archiveDocument), std::move(callback),
                    std::move(errorCallback));
            },
        };
    }

private:
    std::vector<std::shared_ptr<ManualImageCandidateList>> m_loads;
};
}

class TestImageNavigationService : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directoryAdjacentImageUsesInjectedProvider();
    void comicBookAdjacentImageUsesInjectedProvider();
    void directArchiveAdjacentImageUsesInjectedProvider();
    void pageNavigationKeepsKnownListWhileRefreshingCurrentImage();
    void pageNavigationStaysUnknownUntilArchiveListCompletes();
    void pageNavigationDoesNotPublishSyntheticBoundaryWhenCurrentMissing();
    void selectPageUpdatesCurrentPageImmediately();
    void snapshotFollowsCanonicalPageNavigation();
    void knownAdjacentNavigationUsesPendingCurrentPage();
    void fallbackAdjacentNavigationPublishesTargetBeforeOpening();
    void pageNavigationUpdatesWhenDirectoryCandidatesChange();
    void externalCurrentImageRemovalOpensNextFallback();
    void externalCurrentImageRemovalOpensPreviousFallback();
    void externalCurrentImageRemovalClearsWithoutFallback();
    void internalDeletionCandidateChangeDoesNotOpenExternalFallback();
    void archivePageNavigationDoesNotSubscribeToDirectoryChanges();
    void cancelAllNavigationCancelsPendingAdjacentImageLoad();
    void cancelAllNavigationStopsPageRefreshWatcher();
    void directoryContainerNavigationOpensFirstImage();
    void emptyContainerReportsNavigationError();
    void invalidArchiveContainerReportsNavigationError();
    void archiveContainerNavigationOpensFirstImage();
};

void TestImageNavigationService::directoryAdjacentImageUsesInjectedProvider()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(localUrl(QStringLiteral("/images/01.png"))),
            imageCandidate(currentUrl),
            imageCandidate(nextUrl),
        });

    QUrl openedUrl;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }));
    service.openAdjacentImage(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(currentUrl)),
        NavigationDirection::Next);

    QCOMPARE(openedUrl, nextUrl);
}

void TestImageNavigationService::comicBookAdjacentImageUsesInjectedProvider()
{
    FakeCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl currentUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    fakeProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(currentUrl),
            imageCandidate(nextUrl),
        });

    QUrl openedUrl;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }));
    service.openAdjacentImage(
        navigationContext(
            KiriView::DisplayedImageLocation::fromImagePageScope(currentUrl, *archiveDocument)),
        NavigationDirection::Next);

    QCOMPARE(openedUrl, nextUrl);
}

void TestImageNavigationService::directArchiveAdjacentImageUsesInjectedProvider()
{
    FakeCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl currentUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    fakeProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(currentUrl),
            imageCandidate(nextUrl),
        });

    QUrl openedUrl;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }));
    service.openAdjacentImage(
        navigationContext(
            KiriView::DisplayedImageLocation::fromImagePageScope(currentUrl, *archiveDocument)),
        NavigationDirection::Next);

    QCOMPARE(openedUrl, nextUrl);
}

void TestImageNavigationService::pageNavigationKeepsKnownListWhileRefreshingCurrentImage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    std::vector<std::pair<int, int>> observedStates;
    KiriView::ImageNavigationService *servicePtr = nullptr;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({}, {}, {}, [&servicePtr, &observedStates]() {
            observedStates.push_back({ servicePtr->currentPageNumber(), servicePtr->imageCount() });
        }));
    servicePtr = &service;

    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(firstUrl)));
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.imageCount(), 3);

    observedStates.clear();
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(secondUrl)));

    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.imageCount(), 3);
    QCOMPARE(static_cast<int>(observedStates.size()), 1);
    QCOMPARE(observedStates.front().first, 2);
    QCOMPARE(observedStates.front().second, 3);
}

void TestImageNavigationService::pageNavigationStaysUnknownUntilArchiveListCompletes()
{
    ManualArchiveImageCandidateProvider candidateProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));

    std::vector<std::pair<int, int>> observedStates;
    KiriView::ImageNavigationService *servicePtr = nullptr;
    KiriView::ImageNavigationService service(nullptr, candidateProvider.provider(),
        navigationCallbacks({}, {}, {}, [&servicePtr, &observedStates]() {
            observedStates.push_back({ servicePtr->currentPageNumber(), servicePtr->imageCount() });
        }));
    servicePtr = &service;

    service.updatePageNavigation(navigationContext(
        KiriView::DisplayedImageLocation::fromImagePageScope(firstUrl, *archiveDocument)));

    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));
    QCOMPARE(candidateProvider.backLoad().imagePageScope.rootUrl(), archiveDocument->rootUrl());
    QCOMPARE(service.currentPageNumber(), 0);
    QCOMPARE(service.imageCount(), 0);
    QVERIFY(observedStates.empty());

    candidateProvider.finishBackLoad({
        imageCandidate(firstUrl),
        imageCandidate(secondUrl),
        imageCandidate(thirdUrl),
    });

    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.imageCount(), 3);
    QCOMPARE(static_cast<int>(observedStates.size()), 1);
    QCOMPARE(observedStates.back().first, 1);
    QCOMPARE(observedStates.back().second, 3);
}

void TestImageNavigationService::pageNavigationDoesNotPublishSyntheticBoundaryWhenCurrentMissing()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));

    std::vector<std::pair<int, int>> observedStates;
    KiriView::ImageNavigationService *servicePtr = nullptr;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({}, {}, {}, [&servicePtr, &observedStates]() {
            observedStates.push_back({ servicePtr->currentPageNumber(), servicePtr->imageCount() });
        }));
    servicePtr = &service;

    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(thirdUrl),
        });
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(firstUrl)));
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.imageCount(), 2);

    observedStates.clear();
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(secondUrl)));

    QCOMPARE(service.currentPageNumber(), 0);
    QCOMPARE(service.imageCount(), 2);
    QVERIFY(!observedStates.empty());
    for (const std::pair<int, int> &state : observedStates) {
        QVERIFY(!(state.first == 1 && state.second == 1));
    }
    QCOMPARE(observedStates.back().first, 0);
    QCOMPARE(observedStates.back().second, 2);

    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(firstUrl)));
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(secondUrl)));
    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.imageCount(), 3);

    observedStates.clear();
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(thirdUrl),
        });
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(secondUrl)));

    QCOMPARE(service.currentPageNumber(), 0);
    QCOMPARE(service.imageCount(), 2);
    QVERIFY(!observedStates.empty());
    QCOMPARE(observedStates.back().first, 0);
    QCOMPARE(observedStates.back().second, 2);
}

void TestImageNavigationService::selectPageUpdatesCurrentPageImmediately()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    int pageNavigationChangeCount = 0;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks(
            {}, {}, {}, [&pageNavigationChangeCount]() { ++pageNavigationChangeCount; }));
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(firstUrl)));

    pageNavigationChangeCount = 0;
    const std::optional<KiriView::ImageNavigationTarget> selectedUrl = service.selectPage(3);

    QVERIFY(selectedUrl.has_value());
    QCOMPARE(selectedUrl->url, thirdUrl);
    QCOMPARE(service.currentPageNumber(), 3);
    QCOMPARE(service.imageCount(), 3);
    QCOMPARE(pageNavigationChangeCount, 1);

    const std::optional<KiriView::ImageNavigationTarget> samePageUrl = service.selectPage(3);
    QVERIFY(!samePageUrl.has_value());
    QCOMPARE(service.currentPageNumber(), 3);
    QCOMPARE(pageNavigationChangeCount, 1);

    const std::optional<KiriView::ImageNavigationTarget> invalidUrl = service.selectPage(4);
    QVERIFY(!invalidUrl.has_value());
    QCOMPARE(service.currentPageNumber(), 3);
    QCOMPARE(pageNavigationChangeCount, 1);
}

void TestImageNavigationService::snapshotFollowsCanonicalPageNavigation()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });

    KiriView::ImageNavigationService service(
        nullptr, fakeProvider.provider(), navigationCallbacks());
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(firstUrl)));

    const KiriView::ImagePageNavigationSnapshot firstSnapshot = service.pageNavigationSnapshot();
    QCOMPARE(firstSnapshot.currentPageNumber(), 1);
    QCOMPARE(firstSnapshot.imageCount(), 2);
    QVERIFY(firstSnapshot.urlAtPage(2).has_value());
    QCOMPARE(*firstSnapshot.urlAtPage(2), secondUrl);

    QVERIFY(service.selectPage(2).has_value());
    const KiriView::ImagePageNavigationSnapshot secondSnapshot = service.pageNavigationSnapshot();
    QCOMPARE(secondSnapshot.currentPageNumber(), 2);
    QCOMPARE(firstSnapshot.currentPageNumber(), 1);
}

void TestImageNavigationService::knownAdjacentNavigationUsesPendingCurrentPage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    std::vector<QUrl> openedUrls;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrls](const QUrl &url) { openedUrls.push_back(url); }));
    const std::optional<KiriView::ImageCandidateListContext> displayedFirstContext
        = navigationContext(KiriView::DisplayedImageLocation::fromUrl(firstUrl));
    service.updatePageNavigation(displayedFirstContext);

    service.openAdjacentImage(displayedFirstContext, NavigationDirection::Next);
    service.openAdjacentImage(displayedFirstContext, NavigationDirection::Next);

    QCOMPARE(openedUrls.size(), std::size_t(2));
    QCOMPARE(openedUrls.at(0), secondUrl);
    QCOMPARE(openedUrls.at(1), thirdUrl);
    QCOMPARE(service.currentPageNumber(), 3);
    QCOMPARE(service.imageCount(), 3);
}

void TestImageNavigationService::fallbackAdjacentNavigationPublishesTargetBeforeOpening()
{
    ManualArchiveImageCandidateProvider candidateProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));

    QUrl openedUrl;
    int currentPageAtOpen = 0;
    int imageCountAtOpen = 0;
    KiriView::ImageNavigationService *servicePtr = nullptr;
    KiriView::ImageNavigationService service(
        nullptr, candidateProvider.provider(), navigationCallbacks([&](const QUrl &url) {
            openedUrl = url;
            currentPageAtOpen = servicePtr->currentPageNumber();
            imageCountAtOpen = servicePtr->imageCount();
        }));
    servicePtr = &service;

    service.openAdjacentImage(
        navigationContext(
            KiriView::DisplayedImageLocation::fromImagePageScope(firstUrl, *archiveDocument)),
        NavigationDirection::Next);

    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));
    candidateProvider.finishBackLoad({
        imageCandidate(firstUrl),
        imageCandidate(secondUrl),
        imageCandidate(thirdUrl),
    });

    QCOMPARE(openedUrl, secondUrl);
    QCOMPARE(currentPageAtOpen, 2);
    QCOMPARE(imageCountAtOpen, 3);
    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.imageCount(), 3);
}

void TestImageNavigationService::pageNavigationUpdatesWhenDirectoryCandidatesChange()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });

    int pageNavigationChangeCount = 0;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks(
            {}, {}, {}, [&pageNavigationChangeCount]() { ++pageNavigationChangeCount; }));
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(firstUrl)));
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.imageCount(), 2);
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(parentUrl), 1);

    pageNavigationChangeCount = 0;
    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.imageCount(), 3);
    QCOMPARE(pageNavigationChangeCount, 1);
}

void TestImageNavigationService::externalCurrentImageRemovalOpensNextFallback()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    QUrl openedUrl;
    int clearCount = 0;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }, {}, {}, {},
            [&clearCount]() { ++clearCount; }));
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(secondUrl)));

    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(thirdUrl),
        });

    QCOMPARE(clearCount, 1);
    QCOMPARE(openedUrl, thirdUrl);
}

void TestImageNavigationService::externalCurrentImageRemovalOpensPreviousFallback()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    QUrl openedUrl;
    int clearCount = 0;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }, {}, {}, {},
            [&clearCount]() { ++clearCount; }));
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(thirdUrl)));

    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });

    QCOMPARE(clearCount, 1);
    QCOMPARE(openedUrl, secondUrl);
}

void TestImageNavigationService::externalCurrentImageRemovalClearsWithoutFallback()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl currentUrl = localUrl(QStringLiteral("/images/01.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(currentUrl),
        });

    QUrl openedUrl;
    int clearCount = 0;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }, {}, {}, {},
            [&clearCount]() { ++clearCount; }));
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(currentUrl)));

    fakeProvider.emitDirectoryImageChanges(parentUrl, {});

    QCOMPARE(clearCount, 1);
    QVERIFY(openedUrl.isEmpty());
}

void TestImageNavigationService::internalDeletionCandidateChangeDoesNotOpenExternalFallback()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    QUrl openedUrl;
    int clearCount = 0;
    bool deletionInProgress = true;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }, {}, {}, {},
            [&clearCount]() { ++clearCount; },
            [&deletionInProgress]() { return deletionInProgress; }));
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(secondUrl)));

    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(thirdUrl),
        });

    QCOMPARE(clearCount, 0);
    QVERIFY(openedUrl.isEmpty());
    QCOMPARE(service.currentPageNumber(), 0);
    QCOMPARE(service.imageCount(), 2);
}

void TestImageNavigationService::archivePageNavigationDoesNotSubscribeToDirectoryChanges()
{
    FakeCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(firstUrl),
        });

    KiriView::ImageNavigationService service(
        nullptr, fakeProvider.provider(), navigationCallbacks());
    service.updatePageNavigation(navigationContext(
        KiriView::DisplayedImageLocation::fromImagePageScope(firstUrl, *archiveDocument)));

    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.imageCount(), 1);
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(), 0);
}

void TestImageNavigationService::cancelAllNavigationCancelsPendingAdjacentImageLoad()
{
    ManualArchiveImageCandidateProvider candidateProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl currentUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));

    QUrl openedUrl;
    KiriView::ImageNavigationService service(nullptr, candidateProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }));
    service.openAdjacentImage(
        navigationContext(
            KiriView::DisplayedImageLocation::fromImagePageScope(currentUrl, *archiveDocument)),
        NavigationDirection::Next);
    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));

    service.cancelAllNavigation();
    QVERIFY(candidateProvider.backLoad().canceled);
    candidateProvider.finishBackLoad({
        imageCandidate(currentUrl),
        imageCandidate(nextUrl),
    });

    QVERIFY(openedUrl.isEmpty());
}

void TestImageNavigationService::cancelAllNavigationStopsPageRefreshWatcher()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
        });

    int pageNavigationChangeCount = 0;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks(
            {}, {}, {}, [&pageNavigationChangeCount]() { ++pageNavigationChangeCount; }));
    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromUrl(firstUrl)));
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(parentUrl), 1);
    QCOMPARE(service.imageCount(), 2);

    service.cancelAllNavigation();
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(parentUrl), 0);

    pageNavigationChangeCount = 0;
    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(secondUrl),
            imageCandidate(thirdUrl),
        });

    QCOMPARE(service.imageCount(), 2);
    QCOMPARE(pageNavigationChangeCount, 0);
}

void TestImageNavigationService::directoryContainerNavigationOpensFirstImage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b/"));
    const QUrl targetImageUrl = localUrl(QStringLiteral("/books/b/01.png"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::Directory),
        });
    fakeProvider.setDirectoryImages(targetContainerUrl,
        {
            imageCandidate(targetImageUrl),
        });

    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({},
            [&openedImageUrl, &openedContainerUrl](const QUrl &imageUrl, const QUrl &containerUrl) {
                openedImageUrl = imageUrl;
                openedContainerUrl = containerUrl;
            }));
    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(openedImageUrl, targetImageUrl);
    QCOMPARE(openedContainerUrl, targetContainerUrl);
}

void TestImageNavigationService::emptyContainerReportsNavigationError()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> targetArchiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(targetArchiveDocument.has_value());
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(
                currentContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
            containerCandidate(
                targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        });
    fakeProvider.setArchiveImages(targetArchiveDocument->rootUrl(), {});

    QUrl errorContainerUrl;
    KiriView::ContainerNavigationError navigationError
        = KiriView::ContainerNavigationError::Generic;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({}, {},
            [&errorContainerUrl, &navigationError](const QUrl &containerUrl,
                KiriView::ContainerNavigationError error, const QString &) {
                errorContainerUrl = containerUrl;
                navigationError = error;
            }));
    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(errorContainerUrl, targetContainerUrl);
    QCOMPARE(navigationError, KiriView::ContainerNavigationError::EmptyContainer);
}

void TestImageNavigationService::invalidArchiveContainerReportsNavigationError()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/not-archive.txt"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(
                currentContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
            containerCandidate(
                targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        });

    QUrl errorContainerUrl;
    KiriView::ContainerNavigationError navigationError
        = KiriView::ContainerNavigationError::Generic;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({}, {},
            [&errorContainerUrl, &navigationError](const QUrl &containerUrl,
                KiriView::ContainerNavigationError error, const QString &) {
                errorContainerUrl = containerUrl;
                navigationError = error;
            }));
    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(errorContainerUrl, targetContainerUrl);
    QCOMPARE(navigationError, KiriView::ContainerNavigationError::InvalidComicBookArchive);
}

void TestImageNavigationService::archiveContainerNavigationOpensFirstImage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ImagePageScopeLocation> targetArchiveDocument
        = KiriView::imagePageScopeLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(targetArchiveDocument.has_value());
    const QUrl targetImageUrl
        = archivePageUrl(targetArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(
                currentContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
            containerCandidate(
                targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        });
    fakeProvider.setArchiveImages(targetArchiveDocument->rootUrl(),
        {
            imageCandidate(targetImageUrl),
        });

    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({},
            [&openedImageUrl, &openedContainerUrl](const QUrl &imageUrl, const QUrl &containerUrl) {
                openedImageUrl = imageUrl;
                openedContainerUrl = containerUrl;
            }));
    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(openedImageUrl, targetImageUrl);
    QCOMPARE(openedContainerUrl, targetContainerUrl);
}

QTEST_GUILESS_MAIN(TestImageNavigationService)

#include "test_imagenavigationservice.moc"
