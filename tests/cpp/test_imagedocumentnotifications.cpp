// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentnotifications.h"
#include "presentation/imagezoomstate.h"

#include <QObject>
#include <QTest>
#include <vector>

class TestImageDocumentNotifications : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void notificationPlansReturnChangesInEmissionOrder();
};

namespace {
void compareChanges(const std::vector<KiriView::ImageDocumentChange> &actual,
    const std::vector<KiriView::ImageDocumentChange> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

}

void TestImageDocumentNotifications::notificationPlansReturnChangesInEmissionOrder()
{
    compareChanges(KiriView::imageDocumentSpreadTransitionNotifications(),
        { KiriView::ImageDocumentChange::PresentationTransitionState,
            KiriView::ImageDocumentChange::Status, KiriView::ImageDocumentChange::Loading,
            KiriView::ImageDocumentChange::Repaint });
    compareChanges(KiriView::imageDocumentDisplayedLocationNotifications(true, true),
        { KiriView::ImageDocumentChange::DisplayedUrl,
            KiriView::ImageDocumentChange::WindowTitleFileName });
    compareChanges(KiriView::imageDocumentDisplayedLocationNotifications(true, false),
        { KiriView::ImageDocumentChange::DisplayedUrl });
    compareChanges(KiriView::imageDocumentDisplayedLocationNotifications(false, true),
        { KiriView::ImageDocumentChange::WindowTitleFileName });
    compareChanges(KiriView::imageDocumentDisplayedLocationNotifications(false, false),
        std::vector<KiriView::ImageDocumentChange> {});
    compareChanges(KiriView::imageDocumentTwoPageModeNotifications(),
        { KiriView::ImageDocumentChange::TwoPageMode, KiriView::ImageDocumentChange::ImageSize,
            KiriView::ImageDocumentChange::DisplaySize, KiriView::ImageDocumentChange::ZoomPercent,
            KiriView::ImageDocumentChange::ZoomMode,
            KiriView::ImageDocumentChange::MaximumManualZoomPercent,
            KiriView::ImageDocumentChange::DisplaySource, KiriView::ImageDocumentChange::Repaint });
    KiriView::ImageZoomChangeSet spreadZoomChanges;
    spreadZoomChanges.zoomModeChanged = true;
    spreadZoomChanges.zoomPercentChanged = true;
    spreadZoomChanges.displaySizeChanged = true;
    spreadZoomChanges.maximumManualZoomPercentChanged = true;
    compareChanges(KiriView::imageDocumentSpreadZoomNotifications(spreadZoomChanges),
        { KiriView::ImageDocumentChange::ZoomMode, KiriView::ImageDocumentChange::ZoomPercent,
            KiriView::ImageDocumentChange::DisplaySize,
            KiriView::ImageDocumentChange::MaximumManualZoomPercent,
            KiriView::ImageDocumentChange::Repaint, KiriView::ImageDocumentChange::TwoPageMode });
    compareChanges(KiriView::imageDocumentSpreadZoomNotifications({}),
        std::vector<KiriView::ImageDocumentChange> {});
    compareChanges(KiriView::imageDocumentRightToLeftReadingNotifications(false),
        { KiriView::ImageDocumentChange::RightToLeftReading,
            KiriView::ImageDocumentChange::Repaint });
    compareChanges(KiriView::imageDocumentRightToLeftReadingNotifications(true),
        { KiriView::ImageDocumentChange::RightToLeftReading, KiriView::ImageDocumentChange::Repaint,
            KiriView::ImageDocumentChange::TwoPageMode });

    KiriView::ImageZoomChangeSet presentationZoomChanges;
    presentationZoomChanges.imageSizeChanged = true;
    presentationZoomChanges.viewportSizeChanged = true;
    presentationZoomChanges.zoomModeChanged = true;
    presentationZoomChanges.zoomPercentChanged = true;
    presentationZoomChanges.displaySizeChanged = true;
    presentationZoomChanges.maximumManualZoomPercentChanged = true;
    compareChanges(KiriView::imageDocumentPresentationZoomNotifications(presentationZoomChanges),
        { KiriView::ImageDocumentChange::ImageSize, KiriView::ImageDocumentChange::ViewportSize,
            KiriView::ImageDocumentChange::ZoomMode, KiriView::ImageDocumentChange::ZoomPercent,
            KiriView::ImageDocumentChange::DisplaySize, KiriView::ImageDocumentChange::Repaint,
            KiriView::ImageDocumentChange::MaximumManualZoomPercent });

    KiriView::ImageZoomChangeSet tileRefreshOnlyChanges;
    tileRefreshOnlyChanges.scheduleVisibleTileDecode = true;
    compareChanges(KiriView::imageDocumentPresentationZoomNotifications(tileRefreshOnlyChanges),
        std::vector<KiriView::ImageDocumentChange> {});
}

QTEST_GUILESS_MAIN(TestImageDocumentNotifications)

#include "test_imagedocumentnotifications.moc"
