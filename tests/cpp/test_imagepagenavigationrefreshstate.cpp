// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "candidate_test_support.h"
#include "navigation/imagepagenavigationrefreshstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>

namespace {
using KiriView::ImageCandidateListContext;
using KiriView::ImagePageNavigationRefreshState;
using KiriView::TestSupport::localUrl;

ImageCandidateListContext directoryContext(const QUrl &currentUrl, const QUrl &directoryUrl)
{
    return ImageCandidateListContext::forDirectory(currentUrl, directoryUrl);
}
}

class TestImagePageNavigationRefreshState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void refreshPreviewKeepsKnownUrlsOnlyForSameSource();
    void pendingRefreshAcceptsOnlyCurrentOperationAndSource();
    void watchedRefreshAcceptsCurrentSourceWithoutOperationId();
    void clearDropsKnownAndPendingRefreshContext();
};

void TestImagePageNavigationRefreshState::refreshPreviewKeepsKnownUrlsOnlyForSameSource()
{
    ImagePageNavigationRefreshState state;
    const QUrl directoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const ImageCandidateListContext firstContext = directoryContext(firstUrl, directoryUrl);
    const ImageCandidateListContext secondContext = directoryContext(secondUrl, directoryUrl);

    QVERIFY(!state.beginRefresh(firstContext, true).keepKnownUrls);
    state.finishRefresh(firstContext);

    QVERIFY(state.shouldKeepExistingWatcherFor(secondContext));
    QVERIFY(state.beginRefresh(secondContext, true).keepKnownUrls);
    QVERIFY(!state.beginRefresh(secondContext, false).keepKnownUrls);
}

void TestImagePageNavigationRefreshState::pendingRefreshAcceptsOnlyCurrentOperationAndSource()
{
    ImagePageNavigationRefreshState state;
    const QUrl firstDirectoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl secondDirectoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl thirdDirectoryUrl = localUrl(QStringLiteral("/other/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdUrl = localUrl(QStringLiteral("/other/03.png"));
    const ImageCandidateListContext firstContext = directoryContext(firstUrl, firstDirectoryUrl);
    const ImageCandidateListContext secondContext = directoryContext(secondUrl, secondDirectoryUrl);
    const ImageCandidateListContext thirdContext = directoryContext(thirdUrl, thirdDirectoryUrl);

    const KiriView::ImagePageNavigationRefreshPreview firstPreview
        = state.beginRefresh(firstContext, false);
    const KiriView::ImagePageNavigationRefreshPreview secondPreview
        = state.beginRefresh(secondContext, false);

    QVERIFY(!state.acceptedPendingRefreshContext(firstPreview.refreshId, firstContext.source())
            .has_value());
    QVERIFY(!state.acceptedPendingRefreshContext(secondPreview.refreshId, thirdContext.source())
            .has_value());
    const std::optional<ImageCandidateListContext> acceptedContext
        = state.acceptedPendingRefreshContext(secondPreview.refreshId, secondContext.source());
    QVERIFY(acceptedContext.has_value());
    QCOMPARE(acceptedContext->currentUrl(), secondUrl);
    QVERIFY(state.finishPendingRefresh(secondPreview.refreshId));
    QVERIFY(!state.acceptedPendingRefreshContext(secondPreview.refreshId, secondContext.source())
            .has_value());
}

void TestImagePageNavigationRefreshState::watchedRefreshAcceptsCurrentSourceWithoutOperationId()
{
    ImagePageNavigationRefreshState state;
    const QUrl directoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/images/02.png"));
    const ImageCandidateListContext firstContext = directoryContext(firstUrl, directoryUrl);
    const ImageCandidateListContext secondContext = directoryContext(secondUrl, directoryUrl);

    state.beginRefresh(firstContext, false);
    state.beginRefresh(secondContext, false);

    const std::optional<ImageCandidateListContext> acceptedContext
        = state.acceptedWatchedRefreshContext(firstContext.source());
    QVERIFY(acceptedContext.has_value());
    QCOMPARE(acceptedContext->currentUrl(), secondUrl);
}

void TestImagePageNavigationRefreshState::clearDropsKnownAndPendingRefreshContext()
{
    ImagePageNavigationRefreshState state;
    const QUrl directoryUrl = localUrl(QStringLiteral("/images/"));
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const ImageCandidateListContext context = directoryContext(firstUrl, directoryUrl);

    state.beginRefresh(context, false);
    state.finishRefresh(context);
    QVERIFY(state.shouldKeepExistingWatcherFor(context));

    state.clear();
    QVERIFY(!state.shouldKeepExistingWatcherFor(context));
    QVERIFY(!state.acceptedWatchedRefreshContext(context.source()).has_value());
}

QTEST_GUILESS_MAIN(TestImagePageNavigationRefreshState)

#include "test_imagepagenavigationrefreshstate.moc"
