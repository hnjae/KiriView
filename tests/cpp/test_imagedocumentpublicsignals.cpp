// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/imagedocumentpublicsignals.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <vector>

class TestImageDocumentPublicSignals : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicSignalPlansReturnSignalsInEmissionOrder();
    void publicSignalBatchPlansDeduplicateDerivedSignalsInEmissionOrder();
    void emitterDispatchesChangeSignalsInProjectionOrder();
};

namespace {
void comparePublicSignals(const std::vector<KiriView::ImageDocumentPublicSignal> &actual,
    const std::vector<KiriView::ImageDocumentPublicSignal> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

KiriView::ImageDocumentPublicSignalOperations recordingOperations(QStringList &events)
{
    KiriView::ImageDocumentPublicSignalOperations operations;
    operations.sourceUrlChanged = [&events]() { events.append(QStringLiteral("sourceUrl")); };
    operations.statusChanged = [&events]() { events.append(QStringLiteral("status")); };
    operations.loadingChanged = [&events]() { events.append(QStringLiteral("loading")); };
    operations.errorStringChanged = [&events]() { events.append(QStringLiteral("errorString")); };
    operations.windowTitleFileNameChanged
        = [&events]() { events.append(QStringLiteral("windowTitleFileName")); };
    operations.displayedUrlChanged = [&events]() { events.append(QStringLiteral("displayedUrl")); };
    operations.imageSizeChanged = [&events]() { events.append(QStringLiteral("imageSize")); };
    operations.viewportSizeChanged = [&events]() { events.append(QStringLiteral("viewportSize")); };
    operations.viewportFrameChanged
        = [&events]() { events.append(QStringLiteral("viewportFrame")); };
    operations.visibleItemRectChanged
        = [&events]() { events.append(QStringLiteral("visibleItemRect")); };
    operations.displaySizeChanged = [&events]() { events.append(QStringLiteral("displaySize")); };
    operations.zoomPercentKnownChanged
        = [&events]() { events.append(QStringLiteral("zoomPercentKnown")); };
    operations.zoomPercentChanged = [&events]() { events.append(QStringLiteral("zoomPercent")); };
    operations.zoomModeChanged = [&events]() { events.append(QStringLiteral("zoomMode")); };
    operations.maximumManualZoomPercentChanged
        = [&events]() { events.append(QStringLiteral("maximumManualZoomPercent")); };
    operations.pageNavigationChanged
        = [&events]() { events.append(QStringLiteral("pageNavigation")); };
    operations.containerNavigationChanged
        = [&events]() { events.append(QStringLiteral("containerNavigation")); };
    operations.fileDeletionInProgressChanged
        = [&events]() { events.append(QStringLiteral("fileDeletionInProgress")); };
    operations.twoPageModeChanged = [&events]() { events.append(QStringLiteral("twoPageMode")); };
    operations.rightToLeftReadingChanged
        = [&events]() { events.append(QStringLiteral("rightToLeftReading")); };
    operations.rotationDegreesChanged
        = [&events]() { events.append(QStringLiteral("rotationDegrees")); };
    operations.mediaScopeChanged = [&events]() { events.append(QStringLiteral("mediaScope")); };
    operations.unsupportedOpenedCollectionVideoChanged
        = [&events]() { events.append(QStringLiteral("unsupportedOpenedCollectionVideo")); };
    operations.repaintRequested = [&events]() { events.append(QStringLiteral("repaint")); };
    return operations;
}
}

void TestImageDocumentPublicSignals::publicSignalPlansReturnSignalsInEmissionOrder()
{
    using Change = KiriView::ImageDocumentChange;
    using Signal = KiriView::ImageDocumentPublicSignal;

    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::SourceUrl), { Signal::SourceUrl });
    comparePublicSignals(KiriView::imageDocumentPublicSignals(Change::Status),
        { Signal::Status, Signal::ZoomPercentKnown });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::Loading), { Signal::Loading });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::ErrorString), { Signal::ErrorString });
    comparePublicSignals(KiriView::imageDocumentPublicSignals(Change::WindowTitleFileName),
        { Signal::WindowTitleFileName });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::DisplayedUrl), { Signal::DisplayedUrl });
    comparePublicSignals(KiriView::imageDocumentPublicSignals(Change::ImageSize),
        { Signal::ImageSize, Signal::ZoomPercentKnown });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::ViewportSize), { Signal::ViewportSize });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::ViewportFrame), { Signal::ViewportFrame });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::VisibleItemRect), { Signal::VisibleItemRect });
    comparePublicSignals(KiriView::imageDocumentPublicSignals(Change::DisplaySize),
        { Signal::DisplaySize, Signal::ZoomPercentKnown });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::ZoomPercent), { Signal::ZoomPercent });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::ZoomMode), { Signal::ZoomMode });
    comparePublicSignals(KiriView::imageDocumentPublicSignals(Change::MaximumManualZoomPercent),
        { Signal::MaximumManualZoomPercent });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::PageNavigation), { Signal::PageNavigation });
    comparePublicSignals(KiriView::imageDocumentPublicSignals(Change::ContainerNavigation),
        { Signal::ContainerNavigation });
    comparePublicSignals(KiriView::imageDocumentPublicSignals(Change::FileDeletionInProgress),
        { Signal::FileDeletionInProgress });
    comparePublicSignals(KiriView::imageDocumentPublicSignals(Change::TwoPageMode),
        { Signal::TwoPageMode, Signal::PageNavigation });
    comparePublicSignals(KiriView::imageDocumentPublicSignals(Change::RightToLeftReading),
        { Signal::RightToLeftReading });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::Rotation), { Signal::RotationDegrees });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::UnsupportedOpenedCollectionVideo),
        { Signal::UnsupportedOpenedCollectionVideo, Signal::ZoomPercentKnown });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::RenderFrame), { Signal::Repaint });
    comparePublicSignals(
        KiriView::imageDocumentPublicSignals(Change::Repaint), { Signal::Repaint });
}

void TestImageDocumentPublicSignals::
    publicSignalBatchPlansDeduplicateDerivedSignalsInEmissionOrder()
{
    using Change = KiriView::ImageDocumentChange;
    using Signal = KiriView::ImageDocumentPublicSignal;

    comparePublicSignals(KiriView::imageDocumentPublicSignalsForChanges(
                             { Change::TwoPageMode, Change::PageNavigation, Change::DisplayedUrl,
                                 Change::Status, Change::Repaint, Change::TwoPageMode }),
        { Signal::TwoPageMode, Signal::PageNavigation, Signal::DisplayedUrl, Signal::Status,
            Signal::ZoomPercentKnown, Signal::Repaint, Signal::MediaScope });
}

void TestImageDocumentPublicSignals::emitterDispatchesChangeSignalsInProjectionOrder()
{
    QStringList events;
    const KiriView::ImageDocumentPublicSignalEmitter emitter(recordingOperations(events));

    emitter.emitChanges({ KiriView::ImageDocumentChange::TwoPageMode,
        KiriView::ImageDocumentChange::PageNavigation });
    emitter.emitSignal(KiriView::ImageDocumentPublicSignal::Repaint);

    QCOMPARE(events,
        QStringList({
            QStringLiteral("twoPageMode"),
            QStringLiteral("pageNavigation"),
            QStringLiteral("repaint"),
        }));
}

QTEST_GUILESS_MAIN(TestImageDocumentPublicSignals)

#include "test_imagedocumentpublicsignals.moc"
