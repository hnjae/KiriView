// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagedocumentstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::localUrl;
}

class TestImageDocumentState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedUrlAndWindowTitleFollowDisplayedImageLocation();
    void containerNavigationAvailabilityFollowsContainerUrl();
    void statusAndLoadingReducersOnlyNotifyWhenChanged();
    void changeBatchQueuesUniqueChangesUntilDestroyed();
    void notificationPlansReturnChangesInEmissionOrder();
};

void TestImageDocumentState::displayedUrlAndWindowTitleFollowDisplayedImageLocation()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentState state(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl localImageUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(localImageUrl));
    QCOMPARE(state.displayedUrl(), localImageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("page.png"));
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::WindowTitleFileName);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstArchivePageUrl
        = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page001.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromArchiveDocument(
        firstArchivePageUrl, *archiveDocument));
    QCOMPARE(state.displayedUrl(), firstArchivePageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.cbz"));
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::WindowTitleFileName);

    const std::size_t changeCount = changes.size();
    const QUrl secondArchivePageUrl
        = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("page002.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromArchiveDocument(
        secondArchivePageUrl, *archiveDocument));
    QCOMPARE(state.displayedUrl(), secondArchivePageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.cbz"));
    QCOMPARE(changes.size(), changeCount + 1);
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::DisplayedUrl);

    state.clearDisplayedImageUrls();
    QCOMPARE(state.displayedUrl(), QUrl());
    QCOMPARE(state.windowTitleFileName(), QString());
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::WindowTitleFileName);
}

void TestImageDocumentState::containerNavigationAvailabilityFollowsContainerUrl()
{
    KiriView::ImageDocumentState state;
    QVERIFY(!state.containerNavigationAvailable());

    state.setContainerNavigationUrl(localUrl(QStringLiteral("/images/")));
    QVERIFY(state.containerNavigationAvailable());

    state.setContainerNavigationUrl(QUrl());
    QVERIFY(!state.containerNavigationAvailable());
}

void TestImageDocumentState::statusAndLoadingReducersOnlyNotifyWhenChanged()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentState state(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    state.setStatus(KiriView::ImageDocumentStatus::Null);
    state.setLoading(false);
    QVERIFY(changes.empty());

    state.setStatus(KiriView::ImageDocumentStatus::Loading);
    QCOMPARE(state.status(), KiriView::ImageDocumentStatus::Loading);
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::Status);

    state.setStatus(KiriView::ImageDocumentStatus::Loading);
    QCOMPARE(changes.size(), std::size_t(1));

    state.setLoading(true);
    QVERIFY(state.loading());
    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::Loading);

    state.setLoading(true);
    QCOMPARE(changes.size(), std::size_t(2));
}

void TestImageDocumentState::changeBatchQueuesUniqueChangesUntilDestroyed()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentState state(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    {
        [[maybe_unused]] auto batch = state.beginChangeBatch();
        state.setLoading(true);
        state.setLoading(true);
        state.setStatus(KiriView::ImageDocumentStatus::Loading);
        QVERIFY(changes.empty());
    }

    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::Status);

    changes.clear();
    {
        [[maybe_unused]] auto batch = state.beginChangeBatch();
        state.setLoading(false);
        {
            [[maybe_unused]] auto nestedBatch = state.beginChangeBatch();
            state.setErrorString(QStringLiteral("failed"));
        }
        QVERIFY(changes.empty());
        state.setStatus(KiriView::ImageDocumentStatus::Error);
    }

    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::ErrorString);
    QCOMPARE(changes.at(2), KiriView::ImageDocumentChange::Status);
}

void TestImageDocumentState::notificationPlansReturnChangesInEmissionOrder()
{
    auto compareChanges = [](const std::vector<KiriView::ImageDocumentChange> &actual,
                              const std::vector<KiriView::ImageDocumentChange> &expected) {
        QCOMPARE(actual.size(), expected.size());
        for (std::size_t index = 0; index < expected.size(); ++index) {
            QCOMPARE(actual.at(index), expected.at(index));
        }
    };

    compareChanges(KiriView::imageDocumentSpreadTransitionNotifications(),
        { KiriView::ImageDocumentChange::Status, KiriView::ImageDocumentChange::Loading,
            KiriView::ImageDocumentChange::Repaint });
    compareChanges(KiriView::imageDocumentTwoPageModeNotifications(),
        { KiriView::ImageDocumentChange::TwoPageMode, KiriView::ImageDocumentChange::ImageSize,
            KiriView::ImageDocumentChange::DisplaySize, KiriView::ImageDocumentChange::ZoomPercent,
            KiriView::ImageDocumentChange::ZoomMode,
            KiriView::ImageDocumentChange::MaximumManualZoomPercent,
            KiriView::ImageDocumentChange::Repaint });
    compareChanges(KiriView::imageDocumentSpreadZoomNotifications(),
        { KiriView::ImageDocumentChange::ZoomMode, KiriView::ImageDocumentChange::ZoomPercent,
            KiriView::ImageDocumentChange::DisplaySize,
            KiriView::ImageDocumentChange::MaximumManualZoomPercent,
            KiriView::ImageDocumentChange::Repaint, KiriView::ImageDocumentChange::TwoPageMode });
    compareChanges(KiriView::imageDocumentRightToLeftReadingNotifications(false),
        { KiriView::ImageDocumentChange::RightToLeftReading,
            KiriView::ImageDocumentChange::Repaint });
    compareChanges(KiriView::imageDocumentRightToLeftReadingNotifications(true),
        { KiriView::ImageDocumentChange::RightToLeftReading, KiriView::ImageDocumentChange::Repaint,
            KiriView::ImageDocumentChange::TwoPageMode });
}

QTEST_GUILESS_MAIN(TestImageDocumentState)

#include "test_imagedocumentstate.moc"
