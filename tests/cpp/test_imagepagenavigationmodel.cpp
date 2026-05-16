// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepagenavigationmodel.h"

#include "image_test_support.h"

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
    void refreshSourceGuardsStaleWatcherUpdates();
};

void TestImagePageNavigationModel::refreshCompletionPublishesCanonicalPageState()
{
    ImagePageNavigationModel model;
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const ImageCandidateListSource source
        = ImageCandidateListSource::forDirectory(localUrl(QStringLiteral("/images/")));

    QVERIFY(model.completeRefresh({ first, second }, second, source));

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
    QVERIFY(model.completeRefresh({ first, second }, first, source));

    const ImageCandidateListContext firstContext
        = ImageCandidateListContext::forDirectory(first, directory);
    QVERIFY(model.previewRefresh(firstContext));
    comparePage(model, 1, 2);

    const ImageCandidateListContext secondContext
        = ImageCandidateListContext::forDirectory(second, directory);
    QVERIFY(model.previewRefresh(secondContext));
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
    QVERIFY(model.completeRefresh(
        { first, second }, first, ImageCandidateListSource::forDirectory(firstDirectory)));

    const ImageCandidateListContext nextContext
        = ImageCandidateListContext::forDirectory(second, secondDirectory);
    QVERIFY(model.previewRefresh(nextContext));
    comparePage(model, 0, 0);
    QVERIFY(!model.shouldKeepExistingWatcherFor(nextContext));
}

void TestImagePageNavigationModel::selectionUpdatesCurrentPageWithoutChangingKnownUrls()
{
    ImagePageNavigationModel model;
    const QUrl first = indexedImageUrl(0);
    const QUrl second = indexedImageUrl(1);
    const QUrl third = indexedImageUrl(2);
    QVERIFY(model.completeRefresh({ first, second, third }, second,
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

void TestImagePageNavigationModel::refreshSourceGuardsStaleWatcherUpdates()
{
    ImagePageNavigationModel model;
    const QUrl firstDirectory = localUrl(QStringLiteral("/images/"));
    const QUrl secondDirectory = localUrl(QStringLiteral("/other/"));
    const ImageCandidateListContext firstContext
        = ImageCandidateListContext::forDirectory(indexedImageUrl(0), firstDirectory);
    const ImageCandidateListContext secondContext
        = ImageCandidateListContext::forDirectory(indexedImageUrl(1), secondDirectory);

    QVERIFY(!model.previewRefresh(firstContext));
    QVERIFY(model.isCurrentRefreshSource(firstContext.source()));
    QVERIFY(!model.isCurrentRefreshSource(secondContext.source()));

    QVERIFY(!model.previewRefresh(secondContext));
    QVERIFY(!model.isCurrentRefreshSource(firstContext.source()));
    QVERIFY(model.isCurrentRefreshSource(secondContext.source()));
}

QTEST_GUILESS_MAIN(TestImagePageNavigationModel)

#include "test_imagepagenavigationmodel.moc"
