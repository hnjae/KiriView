// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagedocumentpagenavigationmodel.h"

#include "candidate_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
using kiriview::ImageDocumentPageCandidateListContext;
using kiriview::ImageDocumentPageCandidateListSource;
using kiriview::ImageDocumentPageNavigationModel;
using kiriview::NavigationDirection;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::localUrl;

void comparePage(const ImageDocumentPageNavigationModel &model, int pageNumber, int pageCount)
{
    QCOMPARE(model.currentPageNumber(), pageNumber);
    QCOMPARE(model.pageCount(), pageCount);
}
}

class TestImageDocumentPageNavigationModel : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void refreshCompletionPublishesCanonicalPageState();
    void refreshPreviewReusesKnownSourceAndForcesNotification();
    void refreshPreviewClearsDifferentSource();
    void selectionUpdatesCurrentPageWithoutChangingKnownUrls();
    void pendingRefreshOperationGuardsStaleSameSourceCompletions();
    void watchedRefreshSourceTracksCurrentContext();
    void changedCandidateRefreshReportsCurrentImageRemoval();
    void clearDropsRefreshContexts();
    void snapshotIsStableProjection();
};

void TestImageDocumentPageNavigationModel::refreshCompletionPublishesCanonicalPageState()
{
    ImageDocumentPageNavigationModel model;
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageDocumentPageCandidateListSource source
        = ImageDocumentPageCandidateListSource::forDirectory(localUrl(QStringLiteral("/images/")));

    QVERIFY(model.completeRefresh(
        { imageDocumentPageCandidate(first), imageDocumentPageCandidate(second) }, second, source));

    comparePage(model, 2, 2);
    QVERIFY(model.urlAtPage(1).has_value());
    QCOMPARE(*model.urlAtPage(1), first);
    QVERIFY(model.urlAtPage(2).has_value());
    QCOMPARE(*model.urlAtPage(2), second);
    QVERIFY(!model.urlAtPage(3).has_value());
}

void TestImageDocumentPageNavigationModel::refreshPreviewReusesKnownSourceAndForcesNotification()
{
    ImageDocumentPageNavigationModel model;
    const QUrl directory = localUrl(QStringLiteral("/images/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageDocumentPageCandidateListSource source
        = ImageDocumentPageCandidateListSource::forDirectory(directory);
    QVERIFY(model.completeRefresh(
        { imageDocumentPageCandidate(first), imageDocumentPageCandidate(second) }, first, source));

    const ImageDocumentPageCandidateListContext firstContext
        = ImageDocumentPageCandidateListContext::forDirectory(first, directory);
    const kiriview::ImageDocumentPageNavigationRefreshPlan firstRefresh
        = model.beginRefresh(firstContext);
    QVERIFY(firstRefresh.changed);
    comparePage(model, 1, 2);

    const ImageDocumentPageCandidateListContext secondContext
        = ImageDocumentPageCandidateListContext::forDirectory(second, directory);
    const kiriview::ImageDocumentPageNavigationRefreshPlan secondRefresh
        = model.beginRefresh(secondContext);
    QVERIFY(secondRefresh.changed);
    comparePage(model, 2, 2);
    QVERIFY(model.shouldKeepExistingWatcherFor(secondContext));
}

void TestImageDocumentPageNavigationModel::refreshPreviewClearsDifferentSource()
{
    ImageDocumentPageNavigationModel model;
    const QUrl firstDirectory = localUrl(QStringLiteral("/images/"));
    const QUrl secondDirectory = localUrl(QStringLiteral("/other/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    QVERIFY(model.completeRefresh(
        { imageDocumentPageCandidate(first), imageDocumentPageCandidate(second) }, first,
        ImageDocumentPageCandidateListSource::forDirectory(firstDirectory)));

    const ImageDocumentPageCandidateListContext nextContext
        = ImageDocumentPageCandidateListContext::forDirectory(second, secondDirectory);
    const kiriview::ImageDocumentPageNavigationRefreshPlan refresh
        = model.beginRefresh(nextContext);
    QVERIFY(refresh.changed);
    comparePage(model, 0, 0);
    QVERIFY(!model.shouldKeepExistingWatcherFor(nextContext));
}

void TestImageDocumentPageNavigationModel::selectionUpdatesCurrentPageWithoutChangingKnownUrls()
{
    ImageDocumentPageNavigationModel model;
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const QUrl third = indexedImageUrl(2);
    QVERIFY(model.completeRefresh(
        { imageDocumentPageCandidate(first), imageDocumentPageCandidate(second),
            imageDocumentPageCandidate(third) },
        second,
        ImageDocumentPageCandidateListSource::forDirectory(localUrl(QStringLiteral("/images/")))));

    const std::optional<kiriview::ImageDocumentPageTarget> selectedFirst = model.selectPage(1);
    QVERIFY(selectedFirst.has_value());
    QCOMPARE(selectedFirst->url, first);
    comparePage(model, 1, 3);
    QVERIFY(!model.selectPage(1).has_value());

    const std::optional<kiriview::ImageDocumentPageTarget> selectedSecond
        = model.selectAdjacentPage(NavigationDirection::Next);
    QVERIFY(selectedSecond.has_value());
    QCOMPARE(selectedSecond->url, second);
    comparePage(model, 2, 3);
    const std::optional<kiriview::ImageDocumentPageTarget> selectedThird
        = model.selectAdjacentPage(NavigationDirection::Next);
    QVERIFY(selectedThird.has_value());
    QCOMPARE(selectedThird->url, third);
    QVERIFY(!model.selectAdjacentPage(NavigationDirection::Next).has_value());
}

void TestImageDocumentPageNavigationModel::pendingRefreshOperationGuardsStaleSameSourceCompletions()
{
    ImageDocumentPageNavigationModel model;
    const QUrl directory = localUrl(QStringLiteral("/images/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageDocumentPageCandidateListContext firstContext
        = ImageDocumentPageCandidateListContext::forDirectory(first, directory);
    const ImageDocumentPageCandidateListContext secondContext
        = ImageDocumentPageCandidateListContext::forDirectory(second, directory);

    const kiriview::ImageDocumentPageNavigationRefreshPlan firstRefresh
        = model.beginRefresh(firstContext);
    const kiriview::ImageDocumentPageNavigationRefreshPlan secondRefresh
        = model.beginRefresh(secondContext);

    const kiriview::ImageDocumentPageNavigationRefreshResult staleResult
        = model.completePendingRefresh(
            { imageDocumentPageCandidate(first) }, firstRefresh.refreshId, firstContext.source());
    QVERIFY(!staleResult.accepted);

    const kiriview::ImageDocumentPageNavigationRefreshResult currentResult
        = model.completePendingRefresh(
            { imageDocumentPageCandidate(first), imageDocumentPageCandidate(second) },
            secondRefresh.refreshId, secondContext.source());
    QVERIFY(currentResult.accepted);
    QVERIFY(currentResult.context.has_value());
    QCOMPARE(currentResult.context->currentUrl(), second);
    comparePage(model, 2, 2);
}

void TestImageDocumentPageNavigationModel::watchedRefreshSourceTracksCurrentContext()
{
    ImageDocumentPageNavigationModel model;
    const QUrl firstDirectory = localUrl(QStringLiteral("/images/"));
    const QUrl secondDirectory = localUrl(QStringLiteral("/other/"));
    const ImageDocumentPageCandidateListContext firstContext
        = ImageDocumentPageCandidateListContext::forDirectory(indexedImageUrl(0), firstDirectory);
    const ImageDocumentPageCandidateListContext secondContext
        = ImageDocumentPageCandidateListContext::forDirectory(indexedImageUrl(1), secondDirectory);

    const kiriview::ImageDocumentPageNavigationRefreshPlan firstRefresh
        = model.beginRefresh(firstContext);
    QVERIFY(!firstRefresh.changed);
    const kiriview::ImageDocumentPageNavigationRefreshResult firstResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageDocumentPageCandidate(indexedImageUrl(0)) }, firstContext.source());
    QVERIFY(firstResult.accepted);
    QVERIFY(firstResult.context.has_value());
    QCOMPARE(firstResult.context->currentUrl(), firstContext.currentUrl());

    const kiriview::ImageDocumentPageNavigationRefreshResult staleResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageDocumentPageCandidate(indexedImageUrl(1)) }, secondContext.source());
    QVERIFY(!staleResult.accepted);

    const kiriview::ImageDocumentPageNavigationRefreshPlan secondRefresh
        = model.beginRefresh(secondContext);
    QVERIFY(secondRefresh.changed);

    const kiriview::ImageDocumentPageNavigationRefreshResult oldSourceResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageDocumentPageCandidate(indexedImageUrl(0)) }, firstContext.source());
    QVERIFY(!oldSourceResult.accepted);

    const kiriview::ImageDocumentPageNavigationRefreshResult secondResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageDocumentPageCandidate(indexedImageUrl(1)) }, secondContext.source());
    QVERIFY(secondResult.accepted);
    QVERIFY(secondResult.context.has_value());
    QCOMPARE(secondResult.context->currentUrl(), secondContext.currentUrl());
}

void TestImageDocumentPageNavigationModel::changedCandidateRefreshReportsCurrentImageRemoval()
{
    ImageDocumentPageNavigationModel model;
    const QUrl directory = localUrl(QStringLiteral("/images/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageDocumentPageCandidateListSource source
        = ImageDocumentPageCandidateListSource::forDirectory(directory);
    QVERIFY(model.completeRefresh(
        { imageDocumentPageCandidate(first), imageDocumentPageCandidate(second) }, first, source));

    const ImageDocumentPageCandidateListContext context
        = ImageDocumentPageCandidateListContext::forDirectory(first, directory);
    const kiriview::ImageDocumentPageNavigationRefreshPlan refreshPlan
        = model.beginRefresh(context);
    QVERIFY(refreshPlan.changed);

    const kiriview::ImageDocumentPageNavigationRefreshResult refreshResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageDocumentPageCandidate(second) }, source);

    QVERIFY(refreshResult.accepted);
    QVERIFY(refreshResult.changed);
    QVERIFY(refreshResult.currentPageRemoved);
    QVERIFY(refreshResult.context.has_value());
    QCOMPARE(refreshResult.context->currentUrl(), first);
    comparePage(model, 0, 1);
}

void TestImageDocumentPageNavigationModel::clearDropsRefreshContexts()
{
    ImageDocumentPageNavigationModel model;
    const QUrl directory = localUrl(QStringLiteral("/images/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageDocumentPageCandidateListSource source
        = ImageDocumentPageCandidateListSource::forDirectory(directory);
    QVERIFY(model.completeRefresh(
        { imageDocumentPageCandidate(first), imageDocumentPageCandidate(second) }, first, source));

    const ImageDocumentPageCandidateListContext context
        = ImageDocumentPageCandidateListContext::forDirectory(first, directory);
    const kiriview::ImageDocumentPageNavigationRefreshPlan refresh = model.beginRefresh(context);
    QVERIFY(refresh.changed);
    QVERIFY(model.shouldKeepExistingWatcherFor(context));

    QVERIFY(model.clear());
    QVERIFY(!model.shouldKeepExistingWatcherFor(context));

    const kiriview::ImageDocumentPageNavigationRefreshResult watchedResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageDocumentPageCandidate(first) }, source);
    QVERIFY(!watchedResult.accepted);

    const kiriview::ImageDocumentPageNavigationRefreshResult pendingResult
        = model.completePendingRefresh(
            { imageDocumentPageCandidate(first) }, refresh.refreshId, source);
    QVERIFY(!pendingResult.accepted);
}

void TestImageDocumentPageNavigationModel::snapshotIsStableProjection()
{
    ImageDocumentPageNavigationModel model;
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const QUrl third = indexedImageUrl(2);
    QVERIFY(model.completeRefresh(
        { imageDocumentPageCandidate(first), imageDocumentPageCandidate(second),
            imageDocumentPageCandidate(third) },
        second,
        ImageDocumentPageCandidateListSource::forDirectory(localUrl(QStringLiteral("/images/")))));

    const kiriview::ImageDocumentPageNavigationSnapshot snapshot = model.snapshot();
    QCOMPARE(snapshot.currentPageNumber(), 2);
    QCOMPARE(snapshot.pageCount(), 3);
    QVERIFY(snapshot.urlAtPage(1).has_value());
    QCOMPARE(*snapshot.urlAtPage(1), first);
    QCOMPARE(snapshot.state.targets.at(0).name, QStringLiteral("00.png"));
    QVERIFY(snapshot.urlAtPage(3).has_value());
    QCOMPARE(*snapshot.urlAtPage(3), third);
    QVERIFY(!snapshot.urlAtPage(0).has_value());
    QVERIFY(!snapshot.urlAtPage(4).has_value());

    QVERIFY(model.selectPage(3).has_value());
    QCOMPARE(model.currentPageNumber(), 3);
    QCOMPARE(snapshot.currentPageNumber(), 2);
}

QTEST_GUILESS_MAIN(TestImageDocumentPageNavigationModel)

#include "test_imagedocumentpagenavigationmodel.moc"
