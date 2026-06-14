// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentstate.h"
#include "document/imageloadfailure.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::localUrl;
}

class TestImageDocumentState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedUrlAndWindowTitleFollowDisplayedImageLocation();
    void displayedImageLocationUsesCanonicalIdentity();
    void containerNavigationAvailabilityFollowsContainerUrl();
    void statusAndLoadingReducersOnlyNotifyWhenChanged();
    void loadFailureStoresDiagnosticsAndPublishesUserMessage();
    void changeBatchQueuesUniqueChangesUntilDestroyed();
    void injectedChangeBatchSharesStateAndRuntimeNotifications();
};

void TestImageDocumentState::displayedUrlAndWindowTitleFollowDisplayedImageLocation()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl localImageUrl = localUrl(QStringLiteral("/images/page.png"));
    const kiriview::DisplayedImageLocation localLocation
        = kiriview::DisplayedImageLocation::fromUrl(localImageUrl);
    state.setDisplayedImageLocation(localLocation);
    QCOMPARE(state.displayedUrl(), localImageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("page.png"));
    QCOMPARE(changes.back(), kiriview::ImageDocumentChange::WindowTitleFileName);
    const std::size_t localChangeCount = changes.size();
    state.setDisplayedImageLocation(localLocation);
    QCOMPARE(changes.size(), localChangeCount);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstArchivePageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("page001.png"));
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
        firstArchivePageUrl, *archiveCollection));
    QCOMPARE(state.displayedUrl(), firstArchivePageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.cbz"));
    QCOMPARE(changes.back(), kiriview::ImageDocumentChange::WindowTitleFileName);

    const std::size_t changeCount = changes.size();
    const QUrl secondArchivePageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("page002.png"));
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
        secondArchivePageUrl, *archiveCollection));
    QCOMPARE(state.displayedUrl(), secondArchivePageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.cbz"));
    QCOMPARE(changes.size(), changeCount + 1);
    QCOMPARE(changes.back(), kiriview::ImageDocumentChange::DisplayedUrl);

    state.clearDisplayedImageLocation();
    QCOMPARE(state.displayedUrl(), QUrl());
    QCOMPARE(state.windowTitleFileName(), QString());
    QCOMPARE(changes.back(), kiriview::ImageDocumentChange::WindowTitleFileName);
}

void TestImageDocumentState::displayedImageLocationUsesCanonicalIdentity()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl rawUrl = localUrl(QStringLiteral("/images/chapter/../page.png"));
    const QUrl normalizedUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(rawUrl));
    QCOMPARE(state.displayedUrl(), normalizedUrl);

    const std::size_t changeCount = changes.size();
    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(normalizedUrl));
    QCOMPARE(state.displayedUrl(), normalizedUrl);
    QCOMPARE(changes.size(), changeCount);
}

void TestImageDocumentState::containerNavigationAvailabilityFollowsContainerUrl()
{
    kiriview::ImageDocumentState state;
    QVERIFY(!state.containerNavigationAvailable());

    state.setContainerNavigationUrl(localUrl(QStringLiteral("/images/")));
    QVERIFY(state.containerNavigationAvailable());

    state.setContainerNavigationUrl(QUrl());
    QVERIFY(!state.containerNavigationAvailable());
}

void TestImageDocumentState::statusAndLoadingReducersOnlyNotifyWhenChanged()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    state.setStatus(kiriview::ImageDocumentStatus::Null);
    state.setLoading(false);
    QVERIFY(changes.empty());

    state.setStatus(kiriview::ImageDocumentStatus::Loading);
    QCOMPARE(state.status(), kiriview::ImageDocumentStatus::Loading);
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.back(), kiriview::ImageDocumentChange::Status);

    state.setStatus(kiriview::ImageDocumentStatus::Loading);
    QCOMPARE(changes.size(), std::size_t(1));

    state.setLoading(true);
    QVERIFY(state.loading());
    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.back(), kiriview::ImageDocumentChange::Loading);

    state.setLoading(true);
    QCOMPARE(changes.size(), std::size_t(2));
}

void TestImageDocumentState::loadFailureStoresDiagnosticsAndPublishesUserMessage()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });
    const QUrl sourceUrl = localUrl(QStringLiteral("/images/missing.png"));

    state.setLoadFailure(kiriview::ImageLoadFailure {
        sourceUrl,
        42,
        kiriview::ImageLoadFailureKind::DataLoad,
        QStringLiteral("Could not read the selected image."),
        QStringLiteral("KIO reported file not found"),
        kiriview::ImageLoadFailureSeverity::Error,
        false,
    });

    QVERIFY(state.loadFailure().has_value());
    QCOMPARE(state.loadFailure()->sourceUrl, sourceUrl);
    QCOMPARE(state.loadFailure()->sessionId, quint64(42));
    QVERIFY(state.loadFailure()->kind == kiriview::ImageLoadFailureKind::DataLoad);
    QCOMPARE(
        state.loadFailure()->userMessage, QStringLiteral("Could not read the selected image."));
    QCOMPARE(state.loadFailure()->diagnosticDetail, QStringLiteral("KIO reported file not found"));
    QVERIFY(state.loadFailure()->severity == kiriview::ImageLoadFailureSeverity::Error);
    QVERIFY(!state.loadFailure()->retryable);
    QCOMPARE(state.errorString(), state.loadFailure()->userMessage);
    QCOMPARE(changes.size(), std::size_t(1));
    QCOMPARE(changes.back(), kiriview::ImageDocumentChange::ErrorString);

    state.setErrorString(QString());

    QVERIFY(!state.loadFailure().has_value());
    QCOMPARE(state.errorString(), QString());
    QCOMPARE(changes.back(), kiriview::ImageDocumentChange::ErrorString);
}

void TestImageDocumentState::changeBatchQueuesUniqueChangesUntilDestroyed()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentState state(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });

    {
        [[maybe_unused]] auto batch = state.beginChangeBatch();
        state.setLoading(true);
        state.setLoading(true);
        state.setStatus(kiriview::ImageDocumentStatus::Loading);
        QVERIFY(changes.empty());
    }

    QCOMPARE(changes.size(), std::size_t(2));
    QCOMPARE(changes.at(0), kiriview::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), kiriview::ImageDocumentChange::Status);

    changes.clear();
    {
        [[maybe_unused]] auto batch = state.beginChangeBatch();
        state.setLoading(false);
        {
            [[maybe_unused]] auto nestedBatch = state.beginChangeBatch();
            state.setErrorString(QStringLiteral("failed"));
        }
        QVERIFY(changes.empty());
        state.setStatus(kiriview::ImageDocumentStatus::Error);
    }

    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), kiriview::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), kiriview::ImageDocumentChange::ErrorString);
    QCOMPARE(changes.at(2), kiriview::ImageDocumentChange::Status);
}

void TestImageDocumentState::injectedChangeBatchSharesStateAndRuntimeNotifications()
{
    std::vector<kiriview::ImageDocumentChange> changes;
    kiriview::ImageDocumentChangeBatcher batcher(
        [&changes](kiriview::ImageDocumentChange change) { changes.push_back(change); });
    kiriview::ImageDocumentState state(batcher);

    {
        [[maybe_unused]] auto batch = batcher.beginBatch();
        state.setLoading(true);
        batcher.notify(kiriview::ImageDocumentChange::Status);
        state.setStatus(kiriview::ImageDocumentStatus::Loading);
        batcher.notify(kiriview::ImageDocumentChange::DisplaySource);
        QVERIFY(changes.empty());
    }

    QCOMPARE(changes.size(), std::size_t(3));
    QCOMPARE(changes.at(0), kiriview::ImageDocumentChange::Loading);
    QCOMPARE(changes.at(1), kiriview::ImageDocumentChange::Status);
    QCOMPARE(changes.at(2), kiriview::ImageDocumentChange::DisplaySource);
}

QTEST_GUILESS_MAIN(TestImageDocumentState)

#include "test_imagedocumentstate.moc"
