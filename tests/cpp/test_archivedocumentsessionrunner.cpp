// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/archivedocumentsessionrunner.h"

#include "archive_session_test_support.h"
#include "image_test_support.h"

#include <QByteArray>
#include <QTest>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace {
using KiriView::ArchiveError;
using KiriView::ArchiveImageCandidates;
using KiriView::ArchiveImageData;
using KiriView::ImageNavigationCandidate;
using KiriView::TestSupport::addInstrumentedArchiveFixture;
using KiriView::TestSupport::archiveDocumentForLocalArchiveUrl;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::instrumentedArchiveSessionFactory;
using KiriView::TestSupport::InstrumentedArchiveSessionState;
using KiriView::TestSupport::localUrl;
}

class TestArchiveDocumentSessionRunner : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsArchiveIdentity();
    void candidateLoadsAreCachedAfterLazyOpen();
    void dataLoadsReuseLazyOpenSession();
    void failedOpenIsMemoized();
};

void TestArchiveDocumentSessionRunner::ownsArchiveIdentity()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveDocument.has_value());

    KiriView::ArchiveDocumentSessionRunner runner(
        *archiveDocument, instrumentedArchiveSessionFactory(state));

    QVERIFY(KiriView::sameImagePageScopeLocation(runner.imagePageScope(), *archiveDocument));
}

void TestArchiveDocumentSessionRunner::candidateLoadsAreCachedAfterLazyOpen()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    addInstrumentedArchiveFixture(
        state, *archiveDocument, { imageCandidate(firstUrl), imageCandidate(secondUrl) });

    KiriView::ArchiveDocumentSessionRunner runner(
        *archiveDocument, instrumentedArchiveSessionFactory(state));

    KiriView::ArchiveImageCandidatesResult firstResult = runner.loadImageCandidates();
    KiriView::ArchiveImageCandidatesResult secondResult = runner.loadImageCandidates();

    const auto *firstCandidates = std::get_if<ArchiveImageCandidates>(&firstResult);
    const auto *secondCandidates = std::get_if<ArchiveImageCandidates>(&secondResult);
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

void TestArchiveDocumentSessionRunner::dataLoadsReuseLazyOpenSession()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedArchiveFixture(state, *archiveDocument, { imageCandidate(pageUrl) });

    KiriView::ArchiveDocumentSessionRunner runner(
        *archiveDocument, instrumentedArchiveSessionFactory(state));

    KiriView::ArchiveImageDataResult firstResult = runner.loadImageData(pageUrl);
    KiriView::ArchiveImageDataResult secondResult = runner.loadImageData(pageUrl);

    const auto *firstData = std::get_if<ArchiveImageData>(&firstResult);
    const auto *secondData = std::get_if<ArchiveImageData>(&secondResult);
    QVERIFY(firstData != nullptr);
    QVERIFY(secondData != nullptr);
    QCOMPARE(firstData->data, QByteArrayLiteral("image"));
    QCOMPARE(secondData->data, QByteArrayLiteral("image"));
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 2);
}

void TestArchiveDocumentSessionRunner::failedOpenIsMemoized()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ImagePageScopeLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/missing.cbz")));
    QVERIFY(archiveDocument.has_value());

    KiriView::ArchiveDocumentSessionRunner runner(
        *archiveDocument, instrumentedArchiveSessionFactory(state));

    KiriView::ArchiveImageCandidatesResult candidatesResult = runner.loadImageCandidates();
    KiriView::ArchiveImageDataResult dataResult = runner.loadImageData(
        archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png")));

    QVERIFY(std::get_if<ArchiveError>(&candidatesResult) != nullptr);
    QVERIFY(std::get_if<ArchiveError>(&dataResult) != nullptr);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 0);
    QCOMPARE(state->dataLoadCount.load(), 0);
}

QTEST_GUILESS_MAIN(TestArchiveDocumentSessionRunner)

#include "test_archivedocumentsessionrunner.moc"
