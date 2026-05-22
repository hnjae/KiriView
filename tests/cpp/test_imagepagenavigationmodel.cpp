// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "navigation/imagepagenavigationmodel.h"

#include "candidate_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
using KiriView::ImageCandidateListContext;
using KiriView::ImageCandidateListSource;
using KiriView::ImagePageNavigationModel;
using KiriView::NavigationDirection;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::localUrl;

void comparePage(const ImagePageNavigationModel &model, int pageNumber, int imageCount)
{
    QCOMPARE(model.currentPageNumber(), pageNumber);
    QCOMPARE(model.imageCount(), imageCount);
}
}

class TestImagePageNavigationModel : public QObject
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

void TestImagePageNavigationModel::refreshCompletionPublishesCanonicalPageState()
{
    ImagePageNavigationModel model;
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageCandidateListSource source
        = ImageCandidateListSource::forDirectory(localUrl(QStringLiteral("/images/")));

    QVERIFY(
        model.completeRefresh({ imageCandidate(first), imageCandidate(second) }, second, source));

    comparePage(model, 2, 2);
    QVERIFY(model.urlAtPage(1).has_value());
    QCOMPARE(*model.urlAtPage(1), first);
    QVERIFY(model.urlAtPage(2).has_value());
    QCOMPARE(*model.urlAtPage(2), second);
    QVERIFY(!model.urlAtPage(3).has_value());
}

void TestImagePageNavigationModel::refreshPreviewReusesKnownSourceAndForcesNotification()
{
    ImagePageNavigationModel model;
    const QUrl directory = localUrl(QStringLiteral("/images/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageCandidateListSource source = ImageCandidateListSource::forDirectory(directory);
    QVERIFY(
        model.completeRefresh({ imageCandidate(first), imageCandidate(second) }, first, source));

    const ImageCandidateListContext firstContext
        = ImageCandidateListContext::forDirectory(first, directory);
    const KiriView::ImagePageNavigationRefreshPlan firstRefresh = model.beginRefresh(firstContext);
    QVERIFY(firstRefresh.changed);
    comparePage(model, 1, 2);

    const ImageCandidateListContext secondContext
        = ImageCandidateListContext::forDirectory(second, directory);
    const KiriView::ImagePageNavigationRefreshPlan secondRefresh
        = model.beginRefresh(secondContext);
    QVERIFY(secondRefresh.changed);
    comparePage(model, 2, 2);
    QVERIFY(model.shouldKeepExistingWatcherFor(secondContext));
}

void TestImagePageNavigationModel::refreshPreviewClearsDifferentSource()
{
    ImagePageNavigationModel model;
    const QUrl firstDirectory = localUrl(QStringLiteral("/images/"));
    const QUrl secondDirectory = localUrl(QStringLiteral("/other/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    QVERIFY(model.completeRefresh({ imageCandidate(first), imageCandidate(second) }, first,
        ImageCandidateListSource::forDirectory(firstDirectory)));

    const ImageCandidateListContext nextContext
        = ImageCandidateListContext::forDirectory(second, secondDirectory);
    const KiriView::ImagePageNavigationRefreshPlan refresh = model.beginRefresh(nextContext);
    QVERIFY(refresh.changed);
    comparePage(model, 0, 0);
    QVERIFY(!model.shouldKeepExistingWatcherFor(nextContext));
}

void TestImagePageNavigationModel::selectionUpdatesCurrentPageWithoutChangingKnownUrls()
{
    ImagePageNavigationModel model;
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const QUrl third = indexedImageUrl(2);
    QVERIFY(model.completeRefresh(
        { imageCandidate(first), imageCandidate(second), imageCandidate(third) }, second,
        ImageCandidateListSource::forDirectory(localUrl(QStringLiteral("/images/")))));

    const std::optional<QUrl> selectedFirst = model.selectPage(1);
    QVERIFY(selectedFirst.has_value());
    QCOMPARE(*selectedFirst, first);
    comparePage(model, 1, 3);
    QVERIFY(!model.selectPage(1).has_value());

    const std::optional<QUrl> selectedSecond = model.selectAdjacentPage(NavigationDirection::Next);
    QVERIFY(selectedSecond.has_value());
    QCOMPARE(*selectedSecond, second);
    comparePage(model, 2, 3);
    const std::optional<QUrl> selectedThird = model.selectAdjacentPage(NavigationDirection::Next);
    QVERIFY(selectedThird.has_value());
    QCOMPARE(*selectedThird, third);
    QVERIFY(!model.selectAdjacentPage(NavigationDirection::Next).has_value());
}

void TestImagePageNavigationModel::pendingRefreshOperationGuardsStaleSameSourceCompletions()
{
    ImagePageNavigationModel model;
    const QUrl directory = localUrl(QStringLiteral("/images/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageCandidateListContext firstContext
        = ImageCandidateListContext::forDirectory(first, directory);
    const ImageCandidateListContext secondContext
        = ImageCandidateListContext::forDirectory(second, directory);

    const KiriView::ImagePageNavigationRefreshPlan firstRefresh = model.beginRefresh(firstContext);
    const KiriView::ImagePageNavigationRefreshPlan secondRefresh
        = model.beginRefresh(secondContext);

    const KiriView::ImagePageNavigationRefreshResult staleResult = model.completePendingRefresh(
        { imageCandidate(first) }, firstRefresh.refreshId, firstContext.source());
    QVERIFY(!staleResult.accepted);

    const KiriView::ImagePageNavigationRefreshResult currentResult
        = model.completePendingRefresh({ imageCandidate(first), imageCandidate(second) },
            secondRefresh.refreshId, secondContext.source());
    QVERIFY(currentResult.accepted);
    QVERIFY(currentResult.context.has_value());
    QCOMPARE(currentResult.context->currentUrl(), second);
    comparePage(model, 2, 2);
}

void TestImagePageNavigationModel::watchedRefreshSourceTracksCurrentContext()
{
    ImagePageNavigationModel model;
    const QUrl firstDirectory = localUrl(QStringLiteral("/images/"));
    const QUrl secondDirectory = localUrl(QStringLiteral("/other/"));
    const ImageCandidateListContext firstContext
        = ImageCandidateListContext::forDirectory(indexedImageUrl(0), firstDirectory);
    const ImageCandidateListContext secondContext
        = ImageCandidateListContext::forDirectory(indexedImageUrl(1), secondDirectory);

    const KiriView::ImagePageNavigationRefreshPlan firstRefresh = model.beginRefresh(firstContext);
    QVERIFY(!firstRefresh.changed);
    const KiriView::ImagePageNavigationRefreshResult firstResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageCandidate(indexedImageUrl(0)) }, firstContext.source());
    QVERIFY(firstResult.accepted);
    QVERIFY(firstResult.context.has_value());
    QCOMPARE(firstResult.context->currentUrl(), firstContext.currentUrl());

    const KiriView::ImagePageNavigationRefreshResult staleResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageCandidate(indexedImageUrl(1)) }, secondContext.source());
    QVERIFY(!staleResult.accepted);

    const KiriView::ImagePageNavigationRefreshPlan secondRefresh
        = model.beginRefresh(secondContext);
    QVERIFY(secondRefresh.changed);

    const KiriView::ImagePageNavigationRefreshResult oldSourceResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageCandidate(indexedImageUrl(0)) }, firstContext.source());
    QVERIFY(!oldSourceResult.accepted);

    const KiriView::ImagePageNavigationRefreshResult secondResult
        = model.completeWatchedRefreshFromCurrentContext(
            { imageCandidate(indexedImageUrl(1)) }, secondContext.source());
    QVERIFY(secondResult.accepted);
    QVERIFY(secondResult.context.has_value());
    QCOMPARE(secondResult.context->currentUrl(), secondContext.currentUrl());
}

void TestImagePageNavigationModel::changedCandidateRefreshReportsCurrentImageRemoval()
{
    ImagePageNavigationModel model;
    const QUrl directory = localUrl(QStringLiteral("/images/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageCandidateListSource source = ImageCandidateListSource::forDirectory(directory);
    QVERIFY(
        model.completeRefresh({ imageCandidate(first), imageCandidate(second) }, first, source));

    const ImageCandidateListContext context
        = ImageCandidateListContext::forDirectory(first, directory);
    const KiriView::ImagePageNavigationRefreshPlan refreshPlan = model.beginRefresh(context);
    QVERIFY(refreshPlan.changed);

    const KiriView::ImagePageNavigationRefreshResult refreshResult
        = model.completeWatchedRefreshFromCurrentContext({ imageCandidate(second) }, source);

    QVERIFY(refreshResult.accepted);
    QVERIFY(refreshResult.changed);
    QVERIFY(refreshResult.currentImageRemoved);
    QVERIFY(refreshResult.context.has_value());
    QCOMPARE(refreshResult.context->currentUrl(), first);
    comparePage(model, 0, 1);
}

void TestImagePageNavigationModel::clearDropsRefreshContexts()
{
    ImagePageNavigationModel model;
    const QUrl directory = localUrl(QStringLiteral("/images/"));
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageCandidateListSource source = ImageCandidateListSource::forDirectory(directory);
    QVERIFY(
        model.completeRefresh({ imageCandidate(first), imageCandidate(second) }, first, source));

    const ImageCandidateListContext context
        = ImageCandidateListContext::forDirectory(first, directory);
    const KiriView::ImagePageNavigationRefreshPlan refresh = model.beginRefresh(context);
    QVERIFY(refresh.changed);
    QVERIFY(model.shouldKeepExistingWatcherFor(context));

    QVERIFY(model.clear());
    QVERIFY(!model.shouldKeepExistingWatcherFor(context));

    const KiriView::ImagePageNavigationRefreshResult watchedResult
        = model.completeWatchedRefreshFromCurrentContext({ imageCandidate(first) }, source);
    QVERIFY(!watchedResult.accepted);

    const KiriView::ImagePageNavigationRefreshResult pendingResult
        = model.completePendingRefresh({ imageCandidate(first) }, refresh.refreshId, source);
    QVERIFY(!pendingResult.accepted);
}

void TestImagePageNavigationModel::snapshotIsStableProjection()
{
    ImagePageNavigationModel model;
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const QUrl third = indexedImageUrl(2);
    QVERIFY(model.completeRefresh(
        { imageCandidate(first), imageCandidate(second), imageCandidate(third) }, second,
        ImageCandidateListSource::forDirectory(localUrl(QStringLiteral("/images/")))));

    const KiriView::ImagePageNavigationSnapshot snapshot = model.snapshot();
    QCOMPARE(snapshot.currentPageNumber(), 2);
    QCOMPARE(snapshot.imageCount(), 3);
    QVERIFY(snapshot.urlAtPage(1).has_value());
    QCOMPARE(*snapshot.urlAtPage(1), first);
    QVERIFY(snapshot.urlAtPage(3).has_value());
    QCOMPARE(*snapshot.urlAtPage(3), third);
    QVERIFY(!snapshot.urlAtPage(0).has_value());
    QVERIFY(!snapshot.urlAtPage(4).has_value());

    QVERIFY(model.selectPage(3).has_value());
    QCOMPARE(model.currentPageNumber(), 3);
    QCOMPARE(snapshot.currentPageNumber(), 2);
}

QTEST_GUILESS_MAIN(TestImagePageNavigationModel)

#include "test_imagepagenavigationmodel.moc"
