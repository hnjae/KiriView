// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentstate.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

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
    void displayedImageLocationUsesCanonicalIdentity();
    void containerNavigationAvailabilityFollowsContainerUrl();
    void statusAndLoadingReducersOnlyNotifyWhenChanged();
    void changeBatchQueuesUniqueChangesUntilDestroyed();
    void injectedChangeBatchSharesStateAndRuntimeNotifications();
};

void TestImageDocumentState::displayedUrlAndWindowTitleFollowDisplayedImageLocation()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentState state(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl localImageUrl = localUrl(QStringLiteral("/images/page.png"));
    const KiriView::DisplayedImageLocation localLocation
        = KiriView::DisplayedImageLocation::fromUrl(localImageUrl);
    state.setDisplayedImageLocation(localLocation);
    QCOMPARE(state.displayedUrl(), localImageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("page.png"));
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::WindowTitleFileName);
    const std::size_t localChangeCount = changes.size();
    state.setDisplayedImageLocation(localLocation);
    QCOMPARE(changes.size(), localChangeCount);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstArchivePageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("page001.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
        firstArchivePageUrl, *archiveCollection));
    QCOMPARE(state.displayedUrl(), firstArchivePageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.cbz"));
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::WindowTitleFileName);

    const std::size_t changeCount = changes.size();
    const QUrl secondArchivePageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("page002.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
        secondArchivePageUrl, *archiveCollection));
    QCOMPARE(state.displayedUrl(), secondArchivePageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.cbz"));
    QCOMPARE(changes.size(), changeCount + 1);
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::DisplayedUrl);

    state.clearDisplayedImageLocation();
    QCOMPARE(state.displayedUrl(), QUrl());
    QCOMPARE(state.windowTitleFileName(), QString());
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::WindowTitleFileName);
}

void TestImageDocumentState::displayedImageLocationUsesCanonicalIdentity()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentState state(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl rawUrl = localUrl(QStringLiteral("/images/chapter/../page.png"));
    const QUrl normalizedUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(rawUrl));
    QCOMPARE(state.displayedUrl(), normalizedUrl);

    const std::size_t changeCount = changes.size();
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(normalizedUrl));
    QCOMPARE(state.displayedUrl(), normalizedUrl);
    QCOMPARE(changes.size(), changeCount);
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

void TestImageDocumentState::injectedChangeBatchSharesStateAndRuntimeNotifications()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentChangeBatcher batcher(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });
    KiriView::ImageDocumentState state(batcher);

    {
        [[maybe_unused]] auto batch = batcher.beginBatch();
        state.setLoading(true);
        batcher.notify(KiriView::ImageDocumentChange::Status);
        state.setStatus(KiriView::ImageDocumentStatus::Loading);
        batcher.notify(KiriView::ImageDocumentChange::DisplaySource);
        QVERIFY(changes.empty());
    }

    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), KiriView::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), KiriView::ImageDocumentChange::Status);
    QCOMPARE(changes.at(2), KiriView::ImageDocumentChange::DisplaySource);
}

QTEST_GUILESS_MAIN(TestImageDocumentState)

#include "test_imagedocumentstate.moc"
