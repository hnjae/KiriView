// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"
#include "imagenavigationservice.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::ContainerNavigationCandidate;
using KiriView::ContainerNavigationCandidateType;
using KiriView::ImageNavigationCandidate;
using KiriView::NavigationDirection;

QString keyForUrl(const QUrl &url) { return url.adjusted(QUrl::NormalizePathSegments).toString(); }

QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

QUrl archivePageUrl(const QUrl &archiveRootUrl, const QString &pageName)
{
    QUrl pageUrl = archiveRootUrl;
    pageUrl.setPath(archiveRootUrl.path() + pageName);
    return pageUrl;
}

ImageNavigationCandidate imageCandidate(const QUrl &url)
{
    return ImageNavigationCandidate { url, url.fileName() };
}

ContainerNavigationCandidate containerCandidate(
    const QUrl &url, ContainerNavigationCandidateType type)
{
    return ContainerNavigationCandidate { url, url.fileName(), type };
}

class FakeCandidateProvider
{
public:
    KiriView::ImageNavigationCandidateProvider provider()
    {
        return KiriView::ImageNavigationCandidateProvider {
            [this](QObject *, QUrl directoryUrl, KiriView::ImageCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                loadImages(directoryImagesByUrl, std::move(directoryUrl), std::move(callback),
                    std::move(errorCallback));
                return KiriView::ImageIoJob();
            },
            [this](QObject *, QUrl directoryUrl, KiriView::ContainerCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                const QString key = keyForUrl(directoryUrl);
                const auto error = containerErrorsByUrl.find(key);
                if (error != containerErrorsByUrl.cend()) {
                    if (errorCallback) {
                        errorCallback(error->second);
                    }
                    return KiriView::ImageIoJob();
                }

                if (callback) {
                    callback(containerCandidatesByUrl[key]);
                }
                return KiriView::ImageIoJob();
            },
            [this](QObject *, KiriView::ArchiveDocumentLocation archiveDocument,
                KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback) {
                loadImages(archiveImagesByUrl, archiveDocument.rootUrl(), std::move(callback),
                    std::move(errorCallback));
                return KiriView::ImageIoJob();
            },
        };
    }

    std::map<QString, std::vector<ImageNavigationCandidate>> directoryImagesByUrl;
    std::map<QString, std::vector<ImageNavigationCandidate>> archiveImagesByUrl;
    std::map<QString, std::vector<ContainerNavigationCandidate>> containerCandidatesByUrl;
    std::map<QString, QString> directoryImageErrorsByUrl;
    std::map<QString, QString> archiveImageErrorsByUrl;
    std::map<QString, QString> containerErrorsByUrl;

private:
    void loadImages(std::map<QString, std::vector<ImageNavigationCandidate>> &imagesByUrl, QUrl url,
        KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
    {
        const QString key = keyForUrl(url);
        const auto &errorsByUrl = &imagesByUrl == &directoryImagesByUrl ? directoryImageErrorsByUrl
                                                                        : archiveImageErrorsByUrl;
        const auto error = errorsByUrl.find(key);
        if (error != errorsByUrl.cend()) {
            if (errorCallback) {
                errorCallback(error->second);
            }
            return;
        }

        if (callback) {
            callback(imagesByUrl[key]);
        }
    }
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
    fakeProvider.directoryImagesByUrl[keyForUrl(parentUrl)] = {
        imageCandidate(localUrl(QStringLiteral("/images/01.png"))),
        imageCandidate(currentUrl),
        imageCandidate(nextUrl),
    };

    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider());
    QUrl openedUrl;
    service.setOpenUrlCallback([&openedUrl](const QUrl &url) { openedUrl = url; });
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
    fakeProvider.archiveImagesByUrl[keyForUrl(archiveDocument->rootUrl())] = {
        imageCandidate(currentUrl),
        imageCandidate(nextUrl),
    };

    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider());
    QUrl openedUrl;
    service.setOpenUrlCallback([&openedUrl](const QUrl &url) { openedUrl = url; });
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
    fakeProvider.archiveImagesByUrl[keyForUrl(archiveDocument->rootUrl())] = {
        imageCandidate(currentUrl),
        imageCandidate(nextUrl),
    };

    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider());
    QUrl openedUrl;
    service.setOpenUrlCallback([&openedUrl](const QUrl &url) { openedUrl = url; });
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
    fakeProvider.directoryImagesByUrl[keyForUrl(parentUrl)] = {
        imageCandidate(firstUrl),
        imageCandidate(secondUrl),
        imageCandidate(thirdUrl),
    };

    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider());
    std::vector<std::pair<int, int>> observedStates;
    service.setPageNavigationChangedCallback([&service, &observedStates]() {
        observedStates.push_back({ service.currentPageNumber(), service.imageCount() });
    });

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
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> targetArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(targetArchiveDocument.has_value());
    const QUrl targetImageUrl
        = archivePageUrl(targetArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.containerCandidatesByUrl[keyForUrl(parentUrl)] = {
        containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
    };
    fakeProvider.archiveImagesByUrl[keyForUrl(targetArchiveDocument->rootUrl())] = {
        imageCandidate(targetImageUrl),
    };

    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider());
    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    service.setOpenContainerImageCallback(
        [&openedImageUrl, &openedContainerUrl](const QUrl &imageUrl, const QUrl &containerUrl) {
            openedImageUrl = imageUrl;
            openedContainerUrl = containerUrl;
        });
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
    fakeProvider.containerCandidatesByUrl[keyForUrl(parentUrl)] = {
        containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
    };
    fakeProvider.archiveImagesByUrl[keyForUrl(targetArchiveDocument->rootUrl())] = {};

    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider());
    QUrl errorContainerUrl;
    KiriView::ContainerNavigationError navigationError
        = KiriView::ContainerNavigationError::Generic;
    service.setContainerNavigationErrorCallback(
        [&errorContainerUrl, &navigationError](
            const QUrl &containerUrl, KiriView::ContainerNavigationError error, const QString &) {
            errorContainerUrl = containerUrl;
            navigationError = error;
        });
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
    fakeProvider.containerCandidatesByUrl[keyForUrl(parentUrl)] = {
        containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
    };

    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider());
    QUrl errorContainerUrl;
    KiriView::ContainerNavigationError navigationError
        = KiriView::ContainerNavigationError::Generic;
    service.setContainerNavigationErrorCallback(
        [&errorContainerUrl, &navigationError](
            const QUrl &containerUrl, KiriView::ContainerNavigationError error, const QString &) {
            errorContainerUrl = containerUrl;
            navigationError = error;
        });
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
    fakeProvider.containerCandidatesByUrl[keyForUrl(parentUrl)] = {
        containerCandidate(currentContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        containerCandidate(targetContainerUrl, ContainerNavigationCandidateType::ComicBookArchive),
    };
    fakeProvider.archiveImagesByUrl[keyForUrl(targetArchiveDocument->rootUrl())] = {
        imageCandidate(targetImageUrl),
    };

    KiriView::ImageNavigationService service(nullptr, fakeProvider.provider());
    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    service.setOpenContainerImageCallback(
        [&openedImageUrl, &openedContainerUrl](const QUrl &imageUrl, const QUrl &containerUrl) {
            openedImageUrl = imageUrl;
            openedContainerUrl = containerUrl;
        });
    service.openAdjacentContainer(currentContainerUrl, NavigationDirection::Next);

    QCOMPARE(openedImageUrl, targetImageUrl);
    QCOMPARE(openedContainerUrl, targetContainerUrl);
}

QTEST_GUILESS_MAIN(TestImageNavigationService)

#include "test_imagenavigationservice.moc"
