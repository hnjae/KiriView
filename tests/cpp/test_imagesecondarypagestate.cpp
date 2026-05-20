// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagesecondarypagestate.h"

#include "image_test_support.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>

namespace {
KiriView::DisplayedImageLocation displayedLocation(const QString &path)
{
    return KiriView::DisplayedImageLocation::fromUrl(KiriView::TestSupport::localUrl(path));
}
}

class TestImageSecondaryPageState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void visibleLoadOwnsDisplayedPage();
    void primaryOnlyLoadClearsDisplayedPageAndRequestsPresentationClear();
    void failedLoadReportsFailureWithoutChangingDisplayedPage();
    void clearDropsDisplayedPage();
};

void TestImageSecondaryPageState::visibleLoadOwnsDisplayedPage()
{
    KiriView::ImageSecondaryPageState state;
    const KiriView::DisplayedImageLocation location
        = displayedLocation(QStringLiteral("/book/002.png"));
    const QSize imageSize(800, 1200);

    const KiriView::ImageSecondaryPageLoadCompletion completion
        = state.finishPresentedLoad(location, imageSize, false);

    QVERIFY(state.visible());
    QCOMPARE(state.displayedImageLocation().imageUrl(), location.imageUrl());
    QCOMPARE(state.imageSize(), imageSize);
    QCOMPARE(completion.result, KiriView::ImageSecondaryPageLoadResult::Visible);
    QCOMPARE(completion.location.imageUrl(), location.imageUrl());
    QCOMPARE(completion.imageSize, imageSize);
    QVERIFY(!completion.clearPresentation);
}

void TestImageSecondaryPageState::primaryOnlyLoadClearsDisplayedPageAndRequestsPresentationClear()
{
    KiriView::ImageSecondaryPageState state;
    state.finishPresentedLoad(
        displayedLocation(QStringLiteral("/book/002.png")), QSize(800, 1200), false);
    const KiriView::DisplayedImageLocation location
        = displayedLocation(QStringLiteral("/book/003.png"));
    const QSize imageSize(1200, 800);

    const KiriView::ImageSecondaryPageLoadCompletion completion
        = state.finishPresentedLoad(location, imageSize, true);

    QVERIFY(!state.visible());
    QVERIFY(state.displayedImageLocation().isEmpty());
    QCOMPARE(state.imageSize(), QSize());
    QCOMPARE(completion.result, KiriView::ImageSecondaryPageLoadResult::PrimaryOnly);
    QCOMPARE(completion.location.imageUrl(), location.imageUrl());
    QCOMPARE(completion.imageSize, imageSize);
    QVERIFY(completion.clearPresentation);
}

void TestImageSecondaryPageState::failedLoadReportsFailureWithoutChangingDisplayedPage()
{
    KiriView::ImageSecondaryPageState state;
    const KiriView::DisplayedImageLocation visibleLocation
        = displayedLocation(QStringLiteral("/book/002.png"));
    const QSize visibleSize(800, 1200);
    state.finishPresentedLoad(visibleLocation, visibleSize, false);
    const KiriView::DisplayedImageLocation failedLocation
        = displayedLocation(QStringLiteral("/book/missing.png"));

    const KiriView::ImageSecondaryPageLoadCompletion completion
        = state.finishFailedLoad(failedLocation);

    QVERIFY(state.visible());
    QCOMPARE(state.displayedImageLocation().imageUrl(), visibleLocation.imageUrl());
    QCOMPARE(state.imageSize(), visibleSize);
    QCOMPARE(completion.result, KiriView::ImageSecondaryPageLoadResult::Failed);
    QCOMPARE(completion.location.imageUrl(), failedLocation.imageUrl());
    QCOMPARE(completion.imageSize, QSize());
    QVERIFY(!completion.clearPresentation);
}

void TestImageSecondaryPageState::clearDropsDisplayedPage()
{
    KiriView::ImageSecondaryPageState state;
    state.finishPresentedLoad(
        displayedLocation(QStringLiteral("/book/002.png")), QSize(800, 1200), false);

    state.clear();

    QVERIFY(!state.visible());
    QVERIFY(state.displayedImageLocation().isEmpty());
    QCOMPARE(state.imageSize(), QSize());
}

QTEST_GUILESS_MAIN(TestImageSecondaryPageState)

#include "test_imagesecondarypagestate.moc"
