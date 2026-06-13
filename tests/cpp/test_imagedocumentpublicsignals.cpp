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
void comparePublicSignals(const std::vector<kiriview::ImageDocumentPublicSignal> &actual,
    const std::vector<kiriview::ImageDocumentPublicSignal> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

kiriview::ImageDocumentPublicSignalOperations recordingOperations(QStringList &events)
{
    kiriview::ImageDocumentPublicSignalOperations operations;
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
    operations.presentationTransitionStateChanged
        = [&events]() { events.append(QStringLiteral("presentationTransitionState")); };
    operations.rotationDegreesChanged
        = [&events]() { events.append(QStringLiteral("rotationDegrees")); };
    operations.imageDocumentSourceScopeChanged
        = [&events]() { events.append(QStringLiteral("imageDocumentSourceScope")); };
    operations.unsupportedOpenedCollectionVideoChanged
        = [&events]() { events.append(QStringLiteral("unsupportedOpenedCollectionVideo")); };
    operations.displaySourceChanged
        = [&events]() { events.append(QStringLiteral("displaySource")); };
    return operations;
}
}

void TestImageDocumentPublicSignals::publicSignalPlansReturnSignalsInEmissionOrder()
{
    using Change = kiriview::ImageDocumentChange;
    using Signal = kiriview::ImageDocumentPublicSignal;

    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::SourceUrl), { Signal::SourceUrl });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::Status),
        { Signal::Status, Signal::ZoomPercentKnown });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::Loading), { Signal::Loading });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::ErrorString), { Signal::ErrorString });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::WindowTitleFileName),
        { Signal::WindowTitleFileName });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::DisplayedUrl), { Signal::DisplayedUrl });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::ImageSize),
        { Signal::ImageSize, Signal::ZoomPercentKnown });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::ViewportSize), { Signal::ViewportSize });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::ViewportFrame), { Signal::ViewportFrame });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::VisibleItemRect), { Signal::VisibleItemRect });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::DisplaySize),
        { Signal::DisplaySize, Signal::ZoomPercentKnown });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::ZoomPercent), { Signal::ZoomPercent });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::ZoomMode), { Signal::ZoomMode });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::MaximumManualZoomPercent),
        { Signal::MaximumManualZoomPercent });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::PageNavigation), { Signal::PageNavigation });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::ContainerNavigation),
        { Signal::ContainerNavigation });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::FileDeletionInProgress),
        { Signal::FileDeletionInProgress });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::TwoPageMode),
        { Signal::TwoPageMode, Signal::PageNavigation });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::RightToLeftReading),
        { Signal::RightToLeftReading });
    comparePublicSignals(kiriview::imageDocumentPublicSignals(Change::PresentationTransitionState),
        { Signal::PresentationTransitionState });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::Rotation), { Signal::RotationDegrees });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::UnsupportedOpenedCollectionVideo),
        { Signal::UnsupportedOpenedCollectionVideo, Signal::ZoomPercentKnown });
    comparePublicSignals(
        kiriview::imageDocumentPublicSignals(Change::DisplaySource), { Signal::DisplaySource });
}

void TestImageDocumentPublicSignals::
    publicSignalBatchPlansDeduplicateDerivedSignalsInEmissionOrder()
{
    using Change = kiriview::ImageDocumentChange;
    using Signal = kiriview::ImageDocumentPublicSignal;

    comparePublicSignals(kiriview::imageDocumentPublicSignalsForChanges(
                             { Change::TwoPageMode, Change::PageNavigation, Change::DisplayedUrl,
                                 Change::Status, Change::DisplaySource, Change::TwoPageMode }),
        { Signal::TwoPageMode, Signal::PageNavigation, Signal::DisplayedUrl, Signal::Status,
            Signal::ZoomPercentKnown, Signal::DisplaySource, Signal::ImageDocumentSourceScope });
}

void TestImageDocumentPublicSignals::emitterDispatchesChangeSignalsInProjectionOrder()
{
    QStringList events;
    const kiriview::ImageDocumentPublicSignalEmitter emitter(recordingOperations(events));

    emitter.emitChanges({ kiriview::ImageDocumentChange::TwoPageMode,
        kiriview::ImageDocumentChange::PageNavigation });
    emitter.emitSignal(kiriview::ImageDocumentPublicSignal::DisplaySource);

    QCOMPARE(events,
        QStringList({
            QStringLiteral("twoPageMode"),
            QStringLiteral("pageNavigation"),
            QStringLiteral("displaySource"),
        }));

    events.clear();
    emitter.emitChange(kiriview::ImageDocumentChange::DisplaySource);
    QCOMPARE(events, QStringList({ QStringLiteral("displaySource") }));
}

QTEST_GUILESS_MAIN(TestImageDocumentPublicSignals)

#include "test_imagedocumentpublicsignals.moc"
