// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"
#include "imagespreadmodecontroller.h"

#include <QTest>
#include <optional>

class TestImageSpreadModeController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void twoPageModeDependsOnCurrentAvailability();
    void rightToLeftReadingResetUsesCurrentModeState();
    void spreadTransitionReportsStateChanges();
};

namespace {
KiriView::DisplayedImageLocation comicPageLocation()
{
    const QUrl archiveUrl = QUrl::fromLocalFile(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    Q_ASSERT(archiveDocument.has_value());

    QUrl pageUrl = archiveDocument->rootUrl();
    pageUrl.setPath(archiveDocument->rootUrl().path() + QStringLiteral("page001.png"));
    return KiriView::DisplayedImageLocation::fromArchiveDocument(pageUrl, *archiveDocument);
}
}

void TestImageSpreadModeController::twoPageModeDependsOnCurrentAvailability()
{
    KiriView::ImageSpreadModeAvailability availability;
    KiriView::ImageSpreadModeController controller([&availability]() { return availability; });

    const KiriView::ImageSpreadTwoPageModeChange change
        = controller.setTwoPageModeEnabled(true, false);
    QVERIFY(change.changed);
    QVERIFY(controller.twoPageModeEnabled());
    QVERIFY(!controller.twoPageModeAvailable());
    QVERIFY(!controller.twoPageModeActive());

    availability.hasImage = true;
    availability.displayedImageLocation = comicPageLocation();
    QVERIFY(controller.twoPageModeAvailable());
    QVERIFY(controller.twoPageModeActive());
}

void TestImageSpreadModeController::rightToLeftReadingResetUsesCurrentModeState()
{
    KiriView::ImageSpreadModeAvailability availability { true, comicPageLocation() };
    KiriView::ImageSpreadModeController controller([&availability]() { return availability; });

    QVERIFY(controller.setRightToLeftReadingEnabled(true));
    QVERIFY(controller.rightToLeftReadingActive());

    const QUrl otherImageUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));
    QVERIFY(controller.shouldResetRightToLeftReadingForLoad(
        availability.displayedImageLocation.archiveDocument(), otherImageUrl, QUrl()));

    controller.resetRightToLeftReading();
    QVERIFY(!controller.rightToLeftReadingEnabled());
    QVERIFY(!controller.shouldResetRightToLeftReadingForLoad(
        availability.displayedImageLocation.archiveDocument(), otherImageUrl, QUrl()));
}

void TestImageSpreadModeController::spreadTransitionReportsStateChanges()
{
    KiriView::ImageSpreadModeController controller({});

    QVERIFY(!controller.spreadTransitionInProgress());
    QVERIFY(controller.beginSpreadTransition());
    QVERIFY(controller.spreadTransitionInProgress());
    QVERIFY(!controller.beginSpreadTransition());
    QVERIFY(controller.finishSpreadTransition());
    QVERIFY(!controller.spreadTransitionInProgress());
    QVERIFY(!controller.finishSpreadTransition());
}

QTEST_GUILESS_MAIN(TestImageSpreadModeController)

#include "test_imagespreadmodecontroller.moc"
