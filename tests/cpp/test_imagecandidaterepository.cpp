// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidaterepository.h"
#include "imagecontainer.h"

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
using KiriView::ImageCandidateRepositoryError;
using KiriView::ImageNavigationCandidate;

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
                loadImages(directoryImagesByUrl, directoryImageErrorsByUrl, std::move(directoryUrl),
                    std::move(callback), std::move(errorCallback));
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
            [this](QObject *, QUrl archiveRootUrl, KiriView::ImageCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                loadImages(archiveImagesByUrl, archiveImageErrorsByUrl, std::move(archiveRootUrl),
                    std::move(callback), std::move(errorCallback));
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
    void loadImages(std::map<QString, std::vector<ImageNavigationCandidate>> &imagesByUrl,
        const std::map<QString, QString> &errorsByUrl, QUrl url,
        KiriView::ImageCandidatesCallback callback, KiriView::ErrorCallback errorCallback)
    {
        const QString key = keyForUrl(url);
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

class TestImageCandidateRepository : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedImageContextsSelectDirectoryOrArchiveListing();
    void directoryContainerOpensFirstImage();
    void archiveContainerOpensFirstImage();
    void emptyContainerReportsError();
    void listingErrorIsForwarded();
};

void TestImageCandidateRepository::displayedImageContextsSelectDirectoryOrArchiveListing()
{
    const QUrl fileUrl = localUrl(QStringLiteral("/images/02.png"));
    const std::optional<KiriView::ImageCandidateListContext> directoryContext
        = KiriView::imageCandidateListContextForDisplayedImage(fileUrl, QUrl());
    QVERIFY(directoryContext.has_value());
    QCOMPARE(directoryContext->currentUrl, fileUrl);
    QCOMPARE(directoryContext->listUrl, localUrl(QStringLiteral("/images/")));
    QCOMPARE(directoryContext->containerType, KiriView::ImageCandidateContainerType::Directory);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl pageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("02.png"));

    const std::optional<KiriView::ImageCandidateListContext> archiveContext
        = KiriView::imageCandidateListContextForDisplayedImage(pageUrl, *archiveRootUrl);
    QVERIFY(archiveContext.has_value());
    QCOMPARE(archiveContext->currentUrl, pageUrl);
    QCOMPARE(archiveContext->listUrl, *archiveRootUrl);
    QCOMPARE(archiveContext->comicBookRootUrl, *archiveRootUrl);
    QCOMPARE(
        archiveContext->containerType, KiriView::ImageCandidateContainerType::ComicBookArchive);
}

void TestImageCandidateRepository::directoryContainerOpensFirstImage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl containerUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl imageUrl = localUrl(QStringLiteral("/books/a/01.png"));
    fakeProvider.directoryImagesByUrl[keyForUrl(containerUrl)] = { imageCandidate(imageUrl) };

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    repository.loadFirstImageInContainer(nullptr,
        containerCandidate(containerUrl, ContainerNavigationCandidateType::Directory),
        [&openedImageUrl, &openedContainerUrl](
            const QUrl &candidateImageUrl, const QUrl &candidateContainerUrl) {
            openedImageUrl = candidateImageUrl;
            openedContainerUrl = candidateContainerUrl;
        },
        {});

    QCOMPARE(openedImageUrl, imageUrl);
    QCOMPARE(openedContainerUrl, containerUrl);
}

void TestImageCandidateRepository::archiveContainerOpensFirstImage()
{
    FakeCandidateProvider fakeProvider;
    const QUrl containerUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(containerUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl imageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("01.png"));
    fakeProvider.archiveImagesByUrl[keyForUrl(*archiveRootUrl)] = { imageCandidate(imageUrl) };

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl openedImageUrl;
    QUrl openedContainerUrl;
    repository.loadFirstImageInContainer(nullptr,
        containerCandidate(containerUrl, ContainerNavigationCandidateType::ComicBookArchive),
        [&openedImageUrl, &openedContainerUrl](
            const QUrl &candidateImageUrl, const QUrl &candidateContainerUrl) {
            openedImageUrl = candidateImageUrl;
            openedContainerUrl = candidateContainerUrl;
        },
        {});

    QCOMPARE(openedImageUrl, imageUrl);
    QCOMPARE(openedContainerUrl, containerUrl);
}

void TestImageCandidateRepository::emptyContainerReportsError()
{
    FakeCandidateProvider fakeProvider;
    const QUrl containerUrl = localUrl(QStringLiteral("/books/a/"));
    fakeProvider.directoryImagesByUrl[keyForUrl(containerUrl)] = {};

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl errorContainerUrl;
    ImageCandidateRepositoryError error = ImageCandidateRepositoryError::Generic;
    repository.loadFirstImageInContainer(nullptr,
        containerCandidate(containerUrl, ContainerNavigationCandidateType::Directory), {},
        [&errorContainerUrl, &error](
            const QUrl &container, ImageCandidateRepositoryError type, const QString &) {
            errorContainerUrl = container;
            error = type;
        });

    QCOMPARE(errorContainerUrl, containerUrl);
    QCOMPARE(error, ImageCandidateRepositoryError::EmptyContainer);
}

void TestImageCandidateRepository::listingErrorIsForwarded()
{
    FakeCandidateProvider fakeProvider;
    const QUrl containerUrl = localUrl(QStringLiteral("/books/a/"));
    fakeProvider.directoryImageErrorsByUrl[keyForUrl(containerUrl)] = QStringLiteral("No access");

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    QUrl errorContainerUrl;
    ImageCandidateRepositoryError error = ImageCandidateRepositoryError::EmptyContainer;
    QString errorString;
    repository.loadFirstImageInContainer(nullptr,
        containerCandidate(containerUrl, ContainerNavigationCandidateType::Directory), {},
        [&errorContainerUrl, &error, &errorString](
            const QUrl &container, ImageCandidateRepositoryError type, const QString &message) {
            errorContainerUrl = container;
            error = type;
            errorString = message;
        });

    QCOMPARE(errorContainerUrl, containerUrl);
    QCOMPARE(error, ImageCandidateRepositoryError::Generic);
    QCOMPARE(errorString, QStringLiteral("No access"));
}

QTEST_GUILESS_MAIN(TestImageCandidateRepository)

#include "test_imagecandidaterepository.moc"
