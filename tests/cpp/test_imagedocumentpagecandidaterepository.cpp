// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagecandidaterepository.h"

#include "candidate_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
using kiriview::ContainerNavigationCandidate;
using kiriview::ContainerNavigationCandidateType;
using kiriview::ImageDocumentPageCandidate;
using kiriview::ImageDocumentPageCandidateListSource;
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::containerCandidate;
using kiriview::TestSupport::FakeImageDocumentPageCandidateProvider;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::localUrl;

void compareSingleImageDocumentPageCandidate(
    const std::vector<ImageDocumentPageCandidate> &candidates, const QUrl &url)
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

class TestImageDocumentPageCandidateRepository : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void loadImagesRoutesDirectorySources();
    void loadImagesRoutesArchiveSources();
    void loadContainersRoutesDirectoryContainerSources();
    void watchCandidateChangesRoutesDirectorySources();
};

void TestImageDocumentPageCandidateRepository::loadImagesRoutesDirectorySources()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl imageUrl = localUrl(QStringLiteral("/books/a/01.png"));
    fakeProvider.setDirectoryImages(directoryUrl, { imageDocumentPageCandidate(imageUrl) });

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    std::vector<ImageDocumentPageCandidate> loadedCandidates;
    repository.loadImages(nullptr, ImageDocumentPageCandidateListSource::forDirectory(directoryUrl),
        [&loadedCandidates](std::vector<ImageDocumentPageCandidate> candidates) {
            loadedCandidates = std::move(candidates);
        },
        {});

    compareSingleImageDocumentPageCandidate(loadedCandidates, imageUrl);
}

void TestImageDocumentPageCandidateRepository::loadImagesRoutesArchiveSources()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> openedCollectionScope
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(openedCollectionScope.has_value());
    const QUrl imageUrl
        = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("01.png"));
    fakeProvider.setOpenedCollectionCandidates(
        openedCollectionScope->rootUrl(), { imageDocumentPageCandidate(imageUrl) });

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    std::vector<ImageDocumentPageCandidate> loadedCandidates;
    repository.loadImages(nullptr,
        ImageDocumentPageCandidateListSource::forOpenedCollectionScope(*openedCollectionScope),
        [&loadedCandidates](std::vector<ImageDocumentPageCandidate> candidates) {
            loadedCandidates = std::move(candidates);
        },
        {});

    compareSingleImageDocumentPageCandidate(loadedCandidates, imageUrl);
}

void TestImageDocumentPageCandidateRepository::loadContainersRoutesDirectoryContainerSources()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/books/"));
    const QUrl containerUrl = localUrl(QStringLiteral("/books/a/"));
    fakeProvider.setContainerCandidates(directoryUrl,
        { containerCandidate(containerUrl, ContainerNavigationCandidateType::Directory) });

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    std::vector<ContainerNavigationCandidate> loadedCandidates;
    repository.loadContainers(nullptr, directoryUrl,
        [&loadedCandidates](std::vector<ContainerNavigationCandidate> candidates) {
            loadedCandidates = std::move(candidates);
        },
        {});

    compareSingleContainerCandidate(loadedCandidates, containerUrl);
}

void TestImageDocumentPageCandidateRepository::watchCandidateChangesRoutesDirectorySources()
{
    FakeImageDocumentPageCandidateProvider fakeProvider;
    const QUrl directoryUrl = localUrl(QStringLiteral("/books/a/"));
    const QUrl imageUrl = localUrl(QStringLiteral("/books/a/01.png"));
    QObject receiver;

    kiriview::ImageDocumentPageCandidateRepository repository(fakeProvider.provider());
    std::vector<ImageDocumentPageCandidate> changedCandidates;
    kiriview::ImageIoJob watchJob = repository.watchCandidateChanges(&receiver,
        ImageDocumentPageCandidateListSource::forDirectory(directoryUrl),
        [&changedCandidates](std::vector<ImageDocumentPageCandidate> candidates) {
            changedCandidates = std::move(candidates);
        },
        {});

    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(directoryUrl), 1);
    fakeProvider.emitDirectoryImageChanges(directoryUrl, { imageDocumentPageCandidate(imageUrl) });
    compareSingleImageDocumentPageCandidate(changedCandidates, imageUrl);

    watchJob.cancel();
    QCOMPARE(fakeProvider.directoryImageChangeSubscriptionCount(directoryUrl), 0);
}

QTEST_GUILESS_MAIN(TestImageDocumentPageCandidateRepository)

#include "test_imagedocumentpagecandidaterepository.moc"
