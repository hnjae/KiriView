// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagecandidaterepository.h"

#include "candidate_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
using KiriView::ContainerNavigationCandidate;
using KiriView::ContainerNavigationCandidateType;
using KiriView::ImageCandidateListSource;
using KiriView::ImageNavigationCandidate;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::containerCandidate;
using KiriView::TestSupport::FakeImageNavigationCandidateProvider;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;

void compareSingleImageCandidate(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &url)
{
    QCOMPARE(candidates.size(), std::size_t(1));
    QCOMPARE(candidates.front().url, url);
}

void compareSingleContainerCandidate(
    const std::vector<ContainerNavigationCandidate> &candidates, const QUrl &url)
{
    QCOMPARE(candidates.size(), std::size_t(1));
    QCOMPARE(candidates.front().url, url);
}
}

class TestImageCandidateRepository : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loadImagesRoutesDirectorySources();
    void loadImagesRoutesArchiveSources();
    void loadContainersRoutesDirectoryContainerSources();
    void watchCandidateChangesRoutesDirectorySources();
};

void TestImageCandidateRepository::loadImagesRoutesDirectorySources()
{
    FakeImageNavigationCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl imageUrl = localUrl(QStringLiteral("/books/a/01.png"));
    fakeProvider.setDirectoryImages(directoryUrl, { imageCandidate(imageUrl) });

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    std::vector<ImageNavigationCandidate> loadedCandidates;
    repository.loadImages(nullptr, ImageCandidateListSource::forDirectory(directoryUrl),
        [&loadedCandidates](std::vector<ImageNavigationCandidate> candidates) {
            loadedCandidates = std::move(candidates);
        },
        {});

    compareSingleImageCandidate(loadedCandidates, imageUrl);
}

void TestImageCandidateRepository::loadImagesRoutesArchiveSources()
{
    FakeImageNavigationCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> openedCollectionScope
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(openedCollectionScope.has_value());
    const QUrl imageUrl
        = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.setOpenedCollectionCandidates(
        openedCollectionScope->rootUrl(), { imageCandidate(imageUrl) });

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    std::vector<ImageNavigationCandidate> loadedCandidates;
    repository.loadImages(nullptr,
        ImageCandidateListSource::forOpenedCollectionScope(*openedCollectionScope),
        [&loadedCandidates](std::vector<ImageNavigationCandidate> candidates) {
            loadedCandidates = std::move(candidates);
        },
        {});

    compareSingleImageCandidate(loadedCandidates, imageUrl);
}

void TestImageCandidateRepository::loadContainersRoutesDirectoryContainerSources()
{
    FakeImageNavigationCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/books/"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/a/"));
    fakeProvider.setContainerCandidates(directoryUrl,
        { containerCandidate(containerUrl, ContainerNavigationCandidateType::Directory) });

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    std::vector<ContainerNavigationCandidate> loadedCandidates;
    repository.loadContainers(nullptr, directoryUrl,
        [&loadedCandidates](std::vector<ContainerNavigationCandidate> candidates) {
            loadedCandidates = std::move(candidates);
        },
        {});

    compareSingleContainerCandidate(loadedCandidates, containerUrl);
}

void TestImageCandidateRepository::watchCandidateChangesRoutesDirectorySources()
{
    FakeImageNavigationCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl imageUrl = localUrl(QStringLiteral("/books/a/01.png"));
    QObject receiver;

    KiriView::ImageCandidateRepository repository(fakeProvider.provider());
    std::vector<ImageNavigationCandidate> changedCandidates;
    KiriView::ImageIoJob watchJob = repository.watchCandidateChanges(&receiver,
        ImageCandidateListSource::forDirectory(directoryUrl),
        [&changedCandidates](std::vector<ImageNavigationCandidate> candidates) {
            changedCandidates = std::move(candidates);
        },
        {});

    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(directoryUrl), 1);
    fakeProvider.emitDirectoryImageChanges(directoryUrl, { imageCandidate(imageUrl) });
    compareSingleImageCandidate(changedCandidates, imageUrl);

    watchJob.cancel();
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(directoryUrl), 0);
}

QTEST_GUILESS_MAIN(TestImageCandidateRepository)

#include "test_imagecandidaterepository.moc"
