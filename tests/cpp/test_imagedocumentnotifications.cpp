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
void compareChanges(const std::vector<kiriview::ImageDocumentChange> &actual,
    const std::vector<kiriview::ImageDocumentChange> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

}

void TestImageDocumentNotifications::notificationPlansReturnChangesInEmissionOrder()
{
    compareChanges(kiriview::imageDocumentSpreadTransitionNotifications(),
        { kiriview::ImageDocumentChange::PresentationTransitionState,
            kiriview::ImageDocumentChange::Status, kiriview::ImageDocumentChange::Loading });
    compareChanges(kiriview::imageDocumentDisplayedLocationNotifications(true, true),
        { kiriview::ImageDocumentChange::DisplayedUrl,
            kiriview::ImageDocumentChange::WindowTitleFileName });
    compareChanges(kiriview::imageDocumentDisplayedLocationNotifications(true, false),
        { kiriview::ImageDocumentChange::DisplayedUrl });
    compareChanges(kiriview::imageDocumentDisplayedLocationNotifications(false, true),
        { kiriview::ImageDocumentChange::WindowTitleFileName });
    compareChanges(kiriview::imageDocumentDisplayedLocationNotifications(false, false),
        std::vector<kiriview::ImageDocumentChange> {});
    compareChanges(kiriview::imageDocumentTwoPageModeNotifications(),
        { kiriview::ImageDocumentChange::TwoPageMode, kiriview::ImageDocumentChange::ImageSize,
            kiriview::ImageDocumentChange::DisplaySize, kiriview::ImageDocumentChange::ZoomPercent,
            kiriview::ImageDocumentChange::ZoomMode,
            kiriview::ImageDocumentChange::MaximumManualZoomPercent,
            kiriview::ImageDocumentChange::DisplaySource });
    kiriview::ImageZoomChangeSet spreadZoomChanges;
    spreadZoomChanges.viewportSizeChanged = true;
    spreadZoomChanges.zoomModeChanged = true;
    spreadZoomChanges.zoomPercentChanged = true;
    spreadZoomChanges.displaySizeChanged = true;
    spreadZoomChanges.maximumManualZoomPercentChanged = true;
    compareChanges(kiriview::imageDocumentSpreadZoomNotifications(spreadZoomChanges),
        { kiriview::ImageDocumentChange::ViewportSize, kiriview::ImageDocumentChange::ZoomMode,
            kiriview::ImageDocumentChange::ZoomPercent, kiriview::ImageDocumentChange::DisplaySize,
            kiriview::ImageDocumentChange::MaximumManualZoomPercent,
            kiriview::ImageDocumentChange::TwoPageMode });
    kiriview::ImageZoomChangeSet viewportOnlySpreadZoomChanges;
    viewportOnlySpreadZoomChanges.viewportSizeChanged = true;
    compareChanges(kiriview::imageDocumentSpreadZoomNotifications(viewportOnlySpreadZoomChanges),
        { kiriview::ImageDocumentChange::ViewportSize,
            kiriview::ImageDocumentChange::DisplaySize });
    compareChanges(kiriview::imageDocumentSpreadZoomNotifications({}),
        std::vector<kiriview::ImageDocumentChange> {});
    compareChanges(kiriview::imageDocumentRightToLeftReadingNotifications(false),
        { kiriview::ImageDocumentChange::RightToLeftReading,
            kiriview::ImageDocumentChange::DisplaySource });
    compareChanges(kiriview::imageDocumentRightToLeftReadingNotifications(true),
        { kiriview::ImageDocumentChange::RightToLeftReading,
            kiriview::ImageDocumentChange::DisplaySource,
            kiriview::ImageDocumentChange::TwoPageMode });

    kiriview::ImageZoomChangeSet presentationZoomChanges;
    presentationZoomChanges.imageSizeChanged = true;
    presentationZoomChanges.viewportSizeChanged = true;
    presentationZoomChanges.zoomModeChanged = true;
    presentationZoomChanges.zoomPercentChanged = true;
    presentationZoomChanges.displaySizeChanged = true;
    presentationZoomChanges.maximumManualZoomPercentChanged = true;
    compareChanges(kiriview::imageDocumentPresentationZoomNotifications(presentationZoomChanges),
        { kiriview::ImageDocumentChange::ImageSize, kiriview::ImageDocumentChange::ViewportSize,
            kiriview::ImageDocumentChange::ZoomMode, kiriview::ImageDocumentChange::ZoomPercent,
            kiriview::ImageDocumentChange::DisplaySize,
            kiriview::ImageDocumentChange::MaximumManualZoomPercent });

    kiriview::ImageZoomChangeSet projectionUpdateOnlyChanges;
    projectionUpdateOnlyChanges.displayProjectionUpdateNeeded = true;
    compareChanges(
        kiriview::imageDocumentPresentationZoomNotifications(projectionUpdateOnlyChanges),
        std::vector<kiriview::ImageDocumentChange> {});
}

QTEST_GUILESS_MAIN(TestImageDocumentNotifications)

#include "test_imagedocumentnotifications.moc"
