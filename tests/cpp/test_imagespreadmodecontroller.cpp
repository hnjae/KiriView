// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationstate.h"
#include "presentation/imagespreadmodecontroller.h"

#include <QTest>

class TestImageSpreadModeController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void twoPageModeRequiresDisplayedComicArchiveImage();
    void resetRightToLeftReadingClearsCurrentModeState();
    void spreadTransitionReportsStateChanges();
    void spreadTransitionPublishesPresentationPhase();
};

namespace {
KiriView::ImageSpreadReadingAvailability readingAvailability(
    bool hasImage, bool hasDisplayedImage, bool displayedDocumentIsComicBook)
{
    return KiriView::ImageSpreadReadingAvailability {
        hasImage,
        hasDisplayedImage,
        displayedDocumentIsComicBook,
    };
}
}

void TestImageSpreadModeController::twoPageModeRequiresDisplayedComicArchiveImage()
{
    KiriView::ImageSpreadModeController controller;

    const KiriView::ImageSpreadTwoPageModeChange change
        = controller.setTwoPageModeEnabled(true, false);
    QVERIFY(change.changed);
    QVERIFY(controller.twoPageModeEnabled());
    QVERIFY(!controller.twoPageModeAvailable(readingAvailability(false, false, false)));
    QVERIFY(!controller.twoPageModeActive(readingAvailability(false, false, false)));

    QVERIFY(!controller.twoPageModeAvailable(readingAvailability(true, false, true)));
    QVERIFY(!controller.twoPageModeActive(readingAvailability(true, false, true)));

    QVERIFY(!controller.twoPageModeAvailable(readingAvailability(true, true, false)));
    QVERIFY(!controller.twoPageModeActive(readingAvailability(true, true, false)));

    QVERIFY(controller.twoPageModeAvailable(readingAvailability(true, true, true)));
    QVERIFY(controller.twoPageModeActive(readingAvailability(true, true, true)));
}

void TestImageSpreadModeController::resetRightToLeftReadingClearsCurrentModeState()
{
    KiriView::ImageSpreadModeController controller;

    QVERIFY(controller.setRightToLeftReadingEnabled(true));
    QVERIFY(controller.rightToLeftReadingActive(readingAvailability(true, true, true)));

    controller.resetRightToLeftReading();
    QVERIFY(!controller.rightToLeftReadingEnabled());
    QVERIFY(!controller.rightToLeftReadingActive(readingAvailability(true, true, true)));
}

void TestImageSpreadModeController::spreadTransitionReportsStateChanges()
{
    KiriView::ImageSpreadModeController controller;

    QVERIFY(!controller.spreadTransitionInProgress());
    QVERIFY(controller.beginSpreadTransition());
    QVERIFY(controller.spreadTransitionInProgress());
    QVERIFY(!controller.beginSpreadTransition());
    QVERIFY(controller.finishSpreadTransition());
    QVERIFY(!controller.spreadTransitionInProgress());
    QVERIFY(!controller.finishSpreadTransition());
}

void TestImageSpreadModeController::spreadTransitionPublishesPresentationPhase()
{
    KiriView::ImageSpreadModeController controller;

    QCOMPARE(controller.presentationTransitionState(),
        KiriView::ImagePresentationTransitionState::CommittedActive);
    QVERIFY(controller.beginSpreadTransition());
    QCOMPARE(controller.presentationTransitionState(),
        KiriView::ImagePresentationTransitionState::TransitioningPlaceholder);
    QVERIFY(controller.finishSpreadTransition());
    QCOMPARE(controller.presentationTransitionState(),
        KiriView::ImagePresentationTransitionState::CommittedActive);
}

QTEST_GUILESS_MAIN(TestImageSpreadModeController)

#include "test_imagespreadmodecontroller.moc"
