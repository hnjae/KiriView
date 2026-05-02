// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"
#include "imagedocumentstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <vector>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

QUrl archivePageUrl(const QUrl &archiveRootUrl, const QString &pageName)
{
    QUrl pageUrl = archiveRootUrl;
    pageUrl.setPath(archiveRootUrl.path() + pageName);
    return pageUrl;
}
}

class TestImageDocumentState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayedUrlAndWindowTitleFollowDisplayedImageLocation();
    void containerNavigationAvailabilityFollowsContainerUrl();
};

void TestImageDocumentState::displayedUrlAndWindowTitleFollowDisplayedImageLocation()
{
    std::vector<KiriView::ImageDocumentChange> changes;
    KiriView::ImageDocumentState state(
        [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); });

    const QUrl localImageUrl = localUrl(QStringLiteral("/images/page.png"));
    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrls(localImageUrl));
    QCOMPARE(state.displayedUrl(), localImageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("page.png"));
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::WindowTitleFileName);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<QUrl> archiveRootUrl = KiriView::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    const QUrl firstArchivePageUrl = archivePageUrl(*archiveRootUrl, QStringLiteral("page001.png"));
    state.setDisplayedImageLocation(
        KiriView::DisplayedImageLocation::fromUrls(firstArchivePageUrl, *archiveRootUrl));
    QCOMPARE(state.displayedUrl(), firstArchivePageUrl);
    QCOMPARE(state.windowTitleFileName(), QStringLiteral("book.cbz"));
    QCOMPARE(changes.back(), KiriView::ImageDocumentChange::WindowTitleFileName);

    const std::size_t changeCount = changes.size();
    const QUrl secondArchivePageUrl
        = archivePageUrl(*archiveRootUrl, QStringLiteral("page002.png"));
    state.setDisplayedImageLocation(
        KiriView::DisplayedImageLocation::fromUrls(secondArchivePageUrl, *archiveRootUrl));
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

QTEST_GUILESS_MAIN(TestImageDocumentState)

#include "test_imagedocumentstate.moc"
