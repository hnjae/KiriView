// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagenavigationservice.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>
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
    KiriView::ImageNavigationService::OpenUrlCallback openUrl = {},
    KiriView::ImageNavigationService::OpenContainerImageCallback openContainerImage = {},
    KiriView::ImageNavigationService::ContainerNavigationErrorCallback containerNavigationError
    = {},
    KiriView::ImageNavigationService::PageNavigationChangedCallback pageNavigationChanged = {},
    KiriView::ImageNavigationService::ClearCurrentImageCallback clearCurrentImage = {},
    KiriView::ImageNavigationService::DeletionInProgressCallback deletionInProgress = {})
{
    return KiriView::ImageNavigationService::Callbacks { std::move(openUrl),
        std::move(openContainerImage), std::move(containerNavigationError),
        std::move(pageNavigationChanged), std::move(clearCurrentImage),
        std::move(deletionInProgress) };
}

struct ManualImageCandidateList {
    QObject *object = nullptr;
    KiriView::ArchiveDocumentLocation archiveDocument;
    KiriView::ImageCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    std::shared_ptr<KiriView::ImageIoJobState> state;
    bool canceled = false;
};

class ManualArchiveImageCandidateProvider
{
public:
    KiriView::ImageIoJob start(QObject *receiver, KiriView::ArchiveDocumentLocation archiveDocument,
        KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualImageCandidateList>();
        load->archiveDocument = std::move(archiveDocument);
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
            [this](QObject *receiver, KiriView::ArchiveDocumentLocation archiveDocument,
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
    void pageNavigationUpdatesWhenDirectoryCandidatesChange();
    void externalCurrentImageRemovalOpensNextFallback();
    void externalCurrentImageRemovalOpensPreviousFallback();
    void externalCurrentImageRemovalClearsWithoutFallback();
    void internalDeletionCandidateChangeDoesNotOpenExternalFallback();
    void archivePageNavigationDoesNotSubscribeToDirectoryChanges();
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
        KiriView::ImageNavigationService::DisplayContext {
            true,
            KiriView::DisplayedImageLocation::fromUrl(currentUrl),
        },
        NavigationDirection::Next);

    QCOMPARE(openedUrl, nextUrl);
}

void TestImageNavigationService::comicBookAdjacentImageUsesInjectedProvider()
{
    FakeCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
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
        KiriView::ImageNavigationService::DisplayContext {
            true,
            KiriView::DisplayedImageLocation::fromArchiveDocument(currentUrl, *archiveDocument),
        },
        NavigationDirection::Next);

    QCOMPARE(openedUrl, nextUrl);
}

void TestImageNavigationService::directArchiveAdjacentImageUsesInjectedProvider()
{
    FakeCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
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
        KiriView::ImageNavigationService::DisplayContext {
            true,
            KiriView::DisplayedImageLocation::fromArchiveDocument(currentUrl, *archiveDocument),
        },
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

    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(firstUrl),
    });
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.imageCount(), 3);

    observedStates.clear();
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(secondUrl),
    });

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
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
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

    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromArchiveDocument(firstUrl, *archiveDocument),
    });

    QCOMPARE(candidateProvider.loadCount(), std::size_t(1));
    QCOMPARE(candidateProvider.backLoad().archiveDocument.rootUrl(), archiveDocument->rootUrl());
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
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(firstUrl),
    });
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.imageCount(), 2);

    observedStates.clear();
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(secondUrl),
    });

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
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(firstUrl),
    });
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(secondUrl),
    });
    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.imageCount(), 3);

    observedStates.clear();
    fakeProvider.setDirectoryImages(parentUrl,
        {
            imageCandidate(firstUrl),
            imageCandidate(thirdUrl),
        });
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(secondUrl),
    });

    QCOMPARE(service.currentPageNumber(), 0);
    QCOMPARE(service.imageCount(), 2);
    QVERIFY(!observedStates.empty());
    QCOMPARE(observedStates.back().first, 0);
    QCOMPARE(observedStates.back().second, 2);
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
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(firstUrl),
    });
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
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(secondUrl),
    });

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
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(thirdUrl),
    });

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
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(currentUrl),
    });

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
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromUrl(secondUrl),
    });

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
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(firstUrl),
        });

    KiriView::ImageNavigationService service(
        nullptr, fakeProvider.provider(), navigationCallbacks());
    service.updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        true,
        KiriView::DisplayedImageLocation::fromArchiveDocument(firstUrl, *archiveDocument),
    });

    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(service.imageCount(), 1);
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(), 0);
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
    const std::optional<KiriView::ArchiveDocumentLocation> targetArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(targetContainerUrl);
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
    const std::optional<KiriView::ArchiveDocumentLocation> targetArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(targetContainerUrl);
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
