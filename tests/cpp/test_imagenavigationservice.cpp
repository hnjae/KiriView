// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagenavigationservice.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::ContainerNavigationCandidateType;
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
    KiriView::ImageNavigationService::PageNavigationChangedCallback pageNavigationChanged = {})
{
    return KiriView::ImageNavigationService::Callbacks { std::move(openUrl),
        std::move(openContainerImage), std::move(containerNavigationError),
        std::move(pageNavigationChanged) };
}
}

class TestImageNavigationService : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void directoryAdjacentImageUsesInjectedProvider();
    void comicBookAdjacentImageUsesInjectedProvider();
    void directArchiveAdjacentImageUsesInjectedProvider();
    void pageNavigationKeepsKnownListWhileRefreshingCurrentImage();
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
