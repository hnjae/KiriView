// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imagecallback.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "navigation/imagedocumentpagenavigationservice.h"

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
using kiriview::ContainerNavigationCandidateType;
using kiriview::ImageDocumentPageCandidate;
using kiriview::NavigationDirection;
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::containerCandidate;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::localUrl;

using FakeCandidateProvider = kiriview::TestSupport::FakeImageDocumentPageCandidateProvider;

kiriview::ImageDocumentPageNavigationService::Callbacks navigationCallbacks(
    std::function<void(const QUrl&)> openUrl = {},
    std::function<void(const QUrl&, const QUrl&)> openContainerImage = {},
    std::function<void(const QUrl&, kiriview::ContainerNavigationError, const QString&)>
        containerNavigationError
    = {},
    kiriview::ImageDocumentPageNavigationService::PageNavigationChangedCallback
        pageNavigationChanged
    = {},
    std::function<void()> clearCurrentImage = {},
    kiriview::ImageDocumentPageNavigationService::DeletionInProgressCallback deletionInProgress
    = {},
    std::function<void(kiriview::NavigationDirection)> containerNavigationBoundary = {})
{
    return kiriview::ImageDocumentPageNavigationService::Callbacks {
        [openUrl = std::move(openUrl), openContainerImage = std::move(openContainerImage),
            containerNavigationError = std::move(containerNavigationError),
            clearCurrentImage = std::move(clearCurrentImage),
            containerNavigationBoundary = std::move(containerNavigationBoundary)](
            kiriview::ImageDocumentPageNavigationPlan plan) mutable {
            for (const kiriview::ImageDocumentPageNavigationEffect& effect : plan) {
                if (const auto* openEffect
                    = std::get_if<kiriview::OpenImageDocumentPageUrlEffect>(&effect)) {
                    kiriview::invokeIfSet(openUrl, openEffect->target.url);
                } else if (const auto* containerEffect
                    = std::get_if<kiriview::OpenContainerImageDocumentPageNavigationEffect>(
                        &effect)) {
                    kiriview::invokeIfSet(openContainerImage, containerEffect->target.url,
                        containerEffect->containerUrl);
                } else if (const auto* errorEffect
                    = std::get_if<kiriview::ReportContainerNavigationErrorEffect>(&effect)) {
                    kiriview::invokeIfSet(containerNavigationError, errorEffect->containerUrl,
                        errorEffect->error, errorEffect->errorString);
                } else if (std::holds_alternative<
                               kiriview::ClearCurrentImageDocumentPageNavigationEffect>(effect)) {
                    kiriview::invokeIfSet(clearCurrentImage);
                } else if (const auto* boundaryEffect
                    = std::get_if<kiriview::ReportContainerNavigationBoundaryEffect>(&effect)) {
                    kiriview::invokeIfSet(containerNavigationBoundary, boundaryEffect->direction);
                }
            }
        },
        std::move(pageNavigationChanged), std::move(deletionInProgress)
    };
}

std::optional<kiriview::ImageDocumentPageCandidateListContext> navigationContext(
    kiriview::DisplayedImageLocation displayedImage)
{
    return kiriview::imageDocumentPageCandidateListContextForDisplayedImage(
        std::move(displayedImage));
}

struct ManualImageDocumentPageCandidateList
{
    QObject* object = nullptr;
    kiriview::OpenedCollectionScopeLocation openedCollectionScope;
    kiriview::ImageDocumentPageCandidatesCallback callback;
    kiriview::ErrorCallback errorCallback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualArchiveImageDocumentPageCandidateProvider
{
public:
    kiriview::ImageIoJob start(QObject* receiver,
        kiriview::OpenedCollectionScopeLocation archiveCollection,
        kiriview::ImageDocumentPageCandidatesCallback callback,
        kiriview::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualImageDocumentPageCandidateList>();
        load->openedCollectionScope = std::move(archiveCollection);
        load->callback = std::move(callback);
        load->errorCallback = std::move(errorCallback);

        kiriview::ImageIoJob job = kiriview::TestSupport::Detail::startManualIoJob(receiver, load);
        m_loads.push_back(load);
        return job;
    }

    std::size_t loadCount() const { return m_loads.size(); }

    ManualImageDocumentPageCandidateList& backLoad() { return *m_loads.back(); }

    void finishBackLoad(std::vector<ImageDocumentPageCandidate> candidates)
    {
        kiriview::TestSupport::Detail::finishManualIoJob(m_loads.back(),
            [candidates = std::move(candidates)](
                ManualImageDocumentPageCandidateList& load) mutable {
                if (load.callback) {
                    load.callback(std::move(candidates));
                }
            });
    }

    kiriview::ImageDocumentPageCandidateProvider provider()
    {
        return kiriview::ImageDocumentPageCandidateProvider {
            [](QObject*, QUrl, kiriview::ImageDocumentPageCandidatesCallback,
                kiriview::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected directory image listing"));
                }
                return kiriview::ImageIoJob();
            },
            [](QObject*, QUrl, kiriview::ContainerCandidatesCallback,
                kiriview::ErrorCallback errorCallback) {
                if (errorCallback) {
                    errorCallback(QStringLiteral("unexpected container listing"));
                }
                return kiriview::ImageIoJob();
            },
            [this](QObject* receiver, kiriview::OpenedCollectionScopeLocation archiveCollection,
                kiriview::ImageDocumentPageCandidatesCallback callback,
                kiriview::ErrorCallback errorCallback) {
                return start(receiver, std::move(archiveCollection), std::move(callback),
                    std::move(errorCallback));
            },
        };
    }

private:
    std::vector<std::shared_ptr<ManualImageDocumentPageCandidateList>> m_loads;
};
}

class TestImageDocumentPageNavigationService : public QObject
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
    void containerNavigationBoundaryPropagates();
    void emptyContainerReportsNavigationError();
    void invalidArchiveContainerReportsNavigationError();
    void archiveContainerNavigationOpensFirstImage();
};

void TestImageDocumentPageNavigationService::directoryAdjacentImageUsesInjectedProvider()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(localUrl(QStringLiteral("/images/01.png"))),
            imageDocumentPageCandidate(currentUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    QUrl openedUrl;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl& url) { openedUrl = url; }));
    service.openAdjacentPage(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(currentUrl)),
        NavigationDirection::Next);

    QCOMPARE(openedUrl, nextUrl);
}

void TestImageDocumentPageNavigationService::comicBookAdjacentImageUsesInjectedProvider()
{
    FakeCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl currentUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    fakeProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(currentUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    QUrl openedUrl;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl& url) { openedUrl = url; }));
    service.openAdjacentPage(
        navigationContext(kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            currentUrl, *archiveCollection)),
        NavigationDirection::Next);

    QCOMPARE(openedUrl, nextUrl);
}

void TestImageDocumentPageNavigationService::directArchiveAdjacentImageUsesInjectedProvider()
{
    FakeCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl currentUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    fakeProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(currentUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    QUrl openedUrl;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl& url) { openedUrl = url; }));
    service.openAdjacentPage(
        navigationContext(kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            currentUrl, *archiveCollection)),
        NavigationDirection::Next);

    QCOMPARE(openedUrl, nextUrl);
}

void TestImageDocumentPageNavigationService::
    pageNavigationKeepsKnownListWhileRefreshingCurrentImage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    std::vector<std::pair<int, int>> observedStates;
    kiriview::ImageDocumentPageNavigationService* servicePtr = nullptr;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({}, {}, {}, [&servicePtr, &observedStates]() {
            observedStates.push_back({ servicePtr->currentPageNumber(), servicePtr->pageCount() });
        }));
    servicePtr = &service;

    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(firstUrl)));
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.pageCount(), 3);

    observedStates.clear();
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(secondUrl)));

    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.pageCount(), 3);
    QCOMPARE(static_cast<int>(observedStates.size()), 1);
    QCOMPARE(observedStates.front().first, 2);
    QCOMPARE(observedStates.front().second, 3);
}

void TestImageDocumentPageNavigationService::pageNavigationStaysUnknownUntilArchiveListCompletes()
{
    ManualArchiveImageDocumentPageCandidateProvider candidateProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));

    std::vector<std::pair<int, int>> observedStates;
    kiriview::ImageDocumentPageNavigationService* servicePtr = nullptr;
    kiriview::ImageDocumentPageNavigationService service(nullptr, candidateProvider.provider(),
        navigationCallbacks({}, {}, {}, [&servicePtr, &observedStates]() {
            observedStates.push_back({ servicePtr->currentPageNumber(), servicePtr->pageCount() });
        }));
    servicePtr = &service;

    service.updatePageNavigation(navigationContext(
        kiriview::DisplayedImageLocation::fromOpenedCollectionScope(firstUrl, *archiveCollection)));

    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));
    QCOMPARE(
        candidateProvider.backLoad().openedCollectionScope.rootUrl(), archiveCollection->rootUrl());
    QCOMPARE(service.currentPageNumber(), 0);
    QCOMPARE(service.pageCount(), 0);
    QVERIFY(observedStates.empty());

    candidateProvider.finishBackLoad({
        imageDocumentPageCandidate(firstUrl),
        imageDocumentPageCandidate(secondUrl),
        imageDocumentPageCandidate(thirdUrl),
    });

    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.pageCount(), 3);
    QCOMPARE(static_cast<int>(observedStates.size()), 1);
    QCOMPARE(observedStates.back().first, 1);
    QCOMPARE(observedStates.back().second, 3);
}

void TestImageDocumentPageNavigationService::
    pageNavigationDoesNotPublishSyntheticBoundaryWhenCurrentMissing()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));

    std::vector<std::pair<int, int>> observedStates;
    kiriview::ImageDocumentPageNavigationService* servicePtr = nullptr;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({}, {}, {}, [&servicePtr, &observedStates]() {
            observedStates.push_back({ servicePtr->currentPageNumber(), servicePtr->pageCount() });
        }));
    servicePtr = &service;

    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(thirdUrl),
        });
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(firstUrl)));
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.pageCount(), 2);

    observedStates.clear();
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(secondUrl)));

    QCOMPARE(service.currentPageNumber(), 0);
    QCOMPARE(service.pageCount(), 2);
    QVERIFY(!observedStates.empty());
    for (const std::pair<int, int>& state : observedStates) {
        QVERIFY(!(state.first == 1 && state.second == 1));
    }
    QCOMPARE(observedStates.back().first, 0);
    QCOMPARE(observedStates.back().second, 2);

    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl),
        });
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(firstUrl)));
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(secondUrl)));
    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.pageCount(), 3);

    observedStates.clear();
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(thirdUrl),
        });
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(secondUrl)));

    QCOMPARE(service.currentPageNumber(), 0);
    QCOMPARE(service.pageCount(), 2);
    QVERIFY(!observedStates.empty());
    QCOMPARE(observedStates.back().first, 0);
    QCOMPARE(observedStates.back().second, 2);
}

void TestImageDocumentPageNavigationService::selectPageUpdatesCurrentPageImmediately()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    int pageNavigationChangeCount = 0;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks(
            {}, {}, {}, [&pageNavigationChangeCount]() { ++pageNavigationChangeCount; }));
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(firstUrl)));

    pageNavigationChangeCount = 0;
    const std::optional<kiriview::ImageDocumentPageTarget> selectedUrl = service.selectPage(3);

    QVERIFY(selectedUrl.has_value());
    QCOMPARE(selectedUrl->url, thirdUrl);
    QCOMPARE(service.currentPageNumber(), 3);
    QCOMPARE(service.pageCount(), 3);
    QCOMPARE(pageNavigationChangeCount, 1);

    const std::optional<kiriview::ImageDocumentPageTarget> samePageUrl = service.selectPage(3);
    QVERIFY(!samePageUrl.has_value());
    QCOMPARE(service.currentPageNumber(), 3);
    QCOMPARE(pageNavigationChangeCount, 1);

    const std::optional<kiriview::ImageDocumentPageTarget> invalidUrl = service.selectPage(4);
    QVERIFY(!invalidUrl.has_value());
    QCOMPARE(service.currentPageNumber(), 3);
    QCOMPARE(pageNavigationChangeCount, 1);
}

void TestImageDocumentPageNavigationService::snapshotFollowsCanonicalPageNavigation()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });

    kiriview::ImageDocumentPageNavigationService service(
        nullptr, fakeProvider.provider(), navigationCallbacks());
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(firstUrl)));

    const kiriview::ImageDocumentPageNavigationSnapshot firstSnapshot
        = service.pageNavigationSnapshot();
    QCOMPARE(firstSnapshot.currentPageNumber(), 1);
    QCOMPARE(firstSnapshot.pageCount(), 2);
    QVERIFY(firstSnapshot.urlAtPage(2).has_value());
    QCOMPARE(*firstSnapshot.urlAtPage(2), secondUrl);

    QVERIFY(service.selectPage(2).has_value());
    const kiriview::ImageDocumentPageNavigationSnapshot secondSnapshot
        = service.pageNavigationSnapshot();
    QCOMPARE(secondSnapshot.currentPageNumber(), 2);
    QCOMPARE(firstSnapshot.currentPageNumber(), 1);
}

void TestImageDocumentPageNavigationService::knownAdjacentNavigationUsesPendingCurrentPage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    std::vector<QUrl> openedUrls;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrls](const QUrl& url) { openedUrls.push_back(url); }));
    const std::optional<kiriview::ImageDocumentPageCandidateListContext> displayedFirstContext
        = navigationContext(kiriview::DisplayedImageLocation::fromUrl(firstUrl));
    service.updatePageNavigation(displayedFirstContext);

    service.openAdjacentPage(displayedFirstContext, NavigationDirection::Next);
    service.openAdjacentPage(displayedFirstContext, NavigationDirection::Next);

    QCOMPARE(openedUrls.size(), std::size_t(2));
    QCOMPARE(openedUrls.at(0), secondUrl);
    QCOMPARE(openedUrls.at(1), thirdUrl);
    QCOMPARE(service.currentPageNumber(), 3);
    QCOMPARE(service.pageCount(), 3);
}

void TestImageDocumentPageNavigationService::
    fallbackAdjacentNavigationPublishesTargetBeforeOpening()
{
    ManualArchiveImageDocumentPageCandidateProvider candidateProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));

    QUrl openedUrl;
    int currentPageAtOpen = 0;
    int pageCountAtOpen = 0;
    kiriview::ImageDocumentPageNavigationService* servicePtr = nullptr;
    kiriview::ImageDocumentPageNavigationService service(
        nullptr, candidateProvider.provider(), navigationCallbacks([&](const QUrl& url) {
            openedUrl = url;
            currentPageAtOpen = servicePtr->currentPageNumber();
            pageCountAtOpen = servicePtr->pageCount();
        }));
    servicePtr = &service;

    service.openAdjacentPage(
        navigationContext(kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            firstUrl, *archiveCollection)),
        NavigationDirection::Next);

    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));
    candidateProvider.finishBackLoad({
        imageDocumentPageCandidate(firstUrl),
        imageDocumentPageCandidate(secondUrl),
        imageDocumentPageCandidate(thirdUrl),
    });

    QCOMPARE(openedUrl, secondUrl);
    QCOMPARE(currentPageAtOpen, 2);
    QCOMPARE(pageCountAtOpen, 3);
    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.pageCount(), 3);
}

void TestImageDocumentPageNavigationService::pageNavigationUpdatesWhenDirectoryCandidatesChange()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });

    int pageNavigationChangeCount = 0;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks(
            {}, {}, {}, [&pageNavigationChangeCount]() { ++pageNavigationChangeCount; }));
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(firstUrl)));
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.pageCount(), 2);
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(parentUrl), 1);

    pageNavigationChangeCount = 0;
    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.pageCount(), 3);
    QCOMPARE(pageNavigationChangeCount, 1);
}

void TestImageDocumentPageNavigationService::externalCurrentImageRemovalOpensNextFallback()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    QUrl openedUrl;
    int clearCount = 0;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl& url) { openedUrl = url; }, {}, {}, {},
            [&clearCount]() { ++clearCount; }));
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(secondUrl)));

    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    QCOMPARE(clearCount, 1);
    QCOMPARE(openedUrl, thirdUrl);
}

void TestImageDocumentPageNavigationService::externalCurrentImageRemovalOpensPreviousFallback()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    QUrl openedUrl;
    int clearCount = 0;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl& url) { openedUrl = url; }, {}, {}, {},
            [&clearCount]() { ++clearCount; }));
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(thirdUrl)));

    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });

    QCOMPARE(clearCount, 1);
    QCOMPARE(openedUrl, secondUrl);
}

void TestImageDocumentPageNavigationService::externalCurrentImageRemovalClearsWithoutFallback()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl currentUrl = localUrl(QStringLiteral("/images/01.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(currentUrl),
        });

    QUrl openedUrl;
    int clearCount = 0;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl& url) { openedUrl = url; }, {}, {}, {},
            [&clearCount]() { ++clearCount; }));
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(currentUrl)));

    fakeProvider.emitDirectoryImageChanges(parentUrl, {});

    QCOMPARE(clearCount, 1);
    QVERIFY(openedUrl.isEmpty());
}

void TestImageDocumentPageNavigationService::
    internalDeletionCandidateChangeDoesNotOpenExternalFallback()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    QUrl openedUrl;
    int clearCount = 0;
    bool deletionInProgress = true;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl& url) { openedUrl = url; }, {}, {}, {},
            [&clearCount]() { ++clearCount; },
            [&deletionInProgress]() { return deletionInProgress; }));
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(secondUrl)));

    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    QCOMPARE(clearCount, 0);
    QVERIFY(openedUrl.isEmpty());
    QCOMPARE(service.currentPageNumber(), 0);
    QCOMPARE(service.pageCount(), 2);
}

void TestImageDocumentPageNavigationService::
    archivePageNavigationDoesNotSubscribeToDirectoryChanges()
{
    FakeCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstUrl),
        });

    kiriview::ImageDocumentPageNavigationService service(
        nullptr, fakeProvider.provider(), navigationCallbacks());
    service.updatePageNavigation(navigationContext(
        kiriview::DisplayedImageLocation::fromOpenedCollectionScope(firstUrl, *archiveCollection)));

    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.pageCount(), 1);
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(), 0);
}

void TestImageDocumentPageNavigationService::cancelAllNavigationCancelsPendingAdjacentImageLoad()
{
    ManualArchiveImageDocumentPageCandidateProvider candidateProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl currentUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));

    QUrl openedUrl;
    kiriview::ImageDocumentPageNavigationService service(nullptr, candidateProvider.provider(),
        navigationCallbacks([&openedUrl](const QUrl& url) { openedUrl = url; }));
    service.openAdjacentPage(
        navigationContext(kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            currentUrl, *archiveCollection)),
        NavigationDirection::Next);
    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));

    service.cancelAllNavigation();
    QVERIFY(candidateProvider.backLoad().canceled);
    candidateProvider.finishBackLoad({
        imageDocumentPageCandidate(currentUrl),
        imageDocumentPageCandidate(nextUrl),
    });

    QVERIFY(openedUrl.isEmpty());
}

void TestImageDocumentPageNavigationService::cancelAllNavigationStopsPageRefreshWatcher()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/images/03.png"));
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
        });

    int pageNavigationChangeCount = 0;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks(
            {}, {}, {}, [&pageNavigationChangeCount]() { ++pageNavigationChangeCount; }));
    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromUrl(firstUrl)));
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(parentUrl), 1);
    QCOMPARE(service.pageCount(), 2);

    service.cancelAllNavigation();
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(parentUrl), 0);

    pageNavigationChangeCount = 0;
    fakeProvider.emitDirectoryImageChanges(parentUrl,
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl),
        });

    QCOMPARE(service.pageCount(), 2);
    QCOMPARE(pageNavigationChangeCount, 0);
}

void TestImageDocumentPageNavigationService::directoryContainerNavigationOpensFirstImage()
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
            imageDocumentPageCandidate(targetImageUrl),
        });

    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({},
            [&openedImageUrl, &openedContainerUrl](const QUrl& imageUrl, const QUrl& containerUrl) {
                openedImageUrl = imageUrl;
                openedContainerUrl = containerUrl;
            }));
    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(openedImageUrl, targetImageUrl);
    QCOMPARE(openedContainerUrl, targetContainerUrl);
}

void TestImageDocumentPageNavigationService::containerNavigationBoundaryPropagates()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a/"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::Directory),
            containerCandidate(
                localUrl(QStringLiteral("/books/b/")), ContainerNavigationCandidateType::Directory),
        });

    int boundaryCount = 0;
    NavigationDirection boundaryDirection = NavigationDirection::Next;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({}, {}, {}, {}, {}, {},
            [&boundaryCount, &boundaryDirection](NavigationDirection direction) {
                ++boundaryCount;
                boundaryDirection = direction;
            }));

    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Previous);

    QCOMPARE(boundaryCount, 1);
    QCOMPARE(boundaryDirection, NavigationDirection::Previous);
}

void TestImageDocumentPageNavigationService::emptyContainerReportsNavigationError()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> targetArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(targetArchiveCollection.has_value());
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(
                currentContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
            containerCandidate(
                targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        });
    fakeProvider.setOpenedCollectionCandidates(targetArchiveCollection->rootUrl(), {});

    QUrl errorContainerUrl;
    kiriview::ContainerNavigationError navigationError
        = kiriview::ContainerNavigationError::Generic;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({}, {},
            [&errorContainerUrl, &navigationError](const QUrl& containerUrl,
                kiriview::ContainerNavigationError error, const QString&) {
                errorContainerUrl = containerUrl;
                navigationError = error;
            }));
    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(errorContainerUrl, targetContainerUrl);
    QCOMPARE(navigationError, kiriview::ContainerNavigationError::EmptyContainer);
}

void TestImageDocumentPageNavigationService::invalidArchiveContainerReportsNavigationError()
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
    kiriview::ContainerNavigationError navigationError
        = kiriview::ContainerNavigationError::Generic;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({}, {},
            [&errorContainerUrl, &navigationError](const QUrl& containerUrl,
                kiriview::ContainerNavigationError error, const QString&) {
                errorContainerUrl = containerUrl;
                navigationError = error;
            }));
    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(errorContainerUrl, targetContainerUrl);
    QCOMPARE(navigationError, kiriview::ContainerNavigationError::InvalidComicBookArchive);
}

void TestImageDocumentPageNavigationService::archiveContainerNavigationOpensFirstImage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl parentUrl = localUrl(QStringLiteral("/books/"));
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> targetArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(targetArchiveCollection.has_value());
    const QUrl targetImageUrl
        = archivePageUrl(targetArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.setContainerCandidates(parentUrl,
        {
            containerCandidate(
                currentContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
            containerCandidate(
                targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        });
    fakeProvider.setOpenedCollectionCandidates(targetArchiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(targetImageUrl),
        });

    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    kiriview::ImageDocumentPageNavigationService service(nullptr, fakeProvider.provider(),
        navigationCallbacks({},
            [&openedImageUrl, &openedContainerUrl](const QUrl& imageUrl, const QUrl& containerUrl) {
                openedImageUrl = imageUrl;
                openedContainerUrl = containerUrl;
            }));
    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(openedImageUrl, targetImageUrl);
    QCOMPARE(openedContainerUrl, targetContainerUrl);
}

QTEST_GUILESS_MAIN(TestImageDocumentPageNavigationService)

#include "test_imagedocumentpagenavigationservice.moc"
