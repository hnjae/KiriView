// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/mediaentrysourcerunner.h"

#include "image_test_support.h"
#include "media_entry_source_test_support.h"

#include <QByteArray>
#include <QTest>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace {
using KiriView::ImageNavigationCandidate;
using KiriView::MediaEntrySourceCandidates;
using KiriView::MediaEntrySourceError;
using KiriView::MediaEntrySourceImageData;
using KiriView::TestSupport::addInstrumentedMediaEntrySourceFixture;
using KiriView::TestSupport::archiveCollectionForLocalArchiveUrl;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::instrumentedMediaEntrySourceFactory;
using KiriView::TestSupport::InstrumentedMediaEntrySourceState;
using KiriView::TestSupport::localUrl;
}

class TestMediaEntrySourceRunner : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsOpenedCollectionScope();
    void candidateLoadsAreCachedAfterLazyOpen();
    void dataLoadsReuseLazyOpenSource();
    void failedOpenIsMemoized();
};

void TestMediaEntrySourceRunner::ownsOpenedCollectionScope()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());

    KiriView::MediaEntrySourceRunner runner(
        *archiveCollection, instrumentedMediaEntrySourceFactory(state));

    QVERIFY(KiriView::sameOpenedCollectionScopeLocation(
        runner.openedCollectionScope(), *archiveCollection));
}

void TestMediaEntrySourceRunner::candidateLoadsAreCachedAfterLazyOpen()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    addInstrumentedMediaEntrySourceFixture(
        state, *archiveCollection, { imageCandidate(firstUrl), imageCandidate(secondUrl) });

    KiriView::MediaEntrySourceRunner runner(
        *archiveCollection, instrumentedMediaEntrySourceFactory(state));

    KiriView::MediaEntrySourceCandidatesResult firstResult = runner.loadImageCandidates();
    KiriView::MediaEntrySourceCandidatesResult secondResult = runner.loadImageCandidates();

    const auto *firstCandidates = std::get_if<MediaEntrySourceCandidates>(&firstResult);
    const auto *secondCandidates = std::get_if<MediaEntrySourceCandidates>(&secondResult);
    QVERIFY(firstCandidates != nullptr);
    QVERIFY(secondCandidates != nullptr);
    QCOMPARE(firstCandidates->candidates.size(), std::size_t(2));
    QCOMPARE(secondCandidates->candidates.size(), std::size_t(2));
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    const std::optional<std::vector<ImageNavigationCandidate>> cached
        = runner.cachedImageCandidates();
    QVERIFY(cached.has_value());
    QCOMPARE(cached->size(), std::size_t(2));
}

void TestMediaEntrySourceRunner::dataLoadsReuseLazyOpenSource()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());
    const QUrl pageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedMediaEntrySourceFixture(state, *archiveCollection, { imageCandidate(pageUrl) });

    KiriView::MediaEntrySourceRunner runner(
        *archiveCollection, instrumentedMediaEntrySourceFactory(state));

    KiriView::MediaEntrySourceImageDataResult firstResult = runner.loadImageData(pageUrl);
    KiriView::MediaEntrySourceImageDataResult secondResult = runner.loadImageData(pageUrl);

    const auto *firstData = std::get_if<MediaEntrySourceImageData>(&firstResult);
    const auto *secondData = std::get_if<MediaEntrySourceImageData>(&secondResult);
    QVERIFY(firstData != nullptr);
    QVERIFY(secondData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("image"));
    QCOMPARE(secondData->data, QByteArrayLiteral("image"));
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 2);
}

void TestMediaEntrySourceRunner::failedOpenIsMemoized()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/missing.cbz")));
    QVERIFY(archiveCollection.has_value());

    KiriView::MediaEntrySourceRunner runner(
        *archiveCollection, instrumentedMediaEntrySourceFactory(state));

    KiriView::MediaEntrySourceCandidatesResult candidatesResult = runner.loadImageCandidates();
    KiriView::MediaEntrySourceImageDataResult dataResult = runner.loadImageData(
        archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png")));

    QVERIFY(std::get_if<MediaEntrySourceError>(&candidatesResult) != nullptr);
    QVERIFY(std::get_if<MediaEntrySourceError>(&dataResult) != nullptr);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 0);
    QCOMPARE(state->dataLoadCount.load(), 0);
}

QTEST_GUILESS_MAIN(TestMediaEntrySourceRunner)

#include "test_mediaentrysourcerunner.moc"
