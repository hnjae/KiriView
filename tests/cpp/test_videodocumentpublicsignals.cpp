// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentpublicsignals.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <vector>

class TestVideoDocumentPublicSignals : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicSignalPlansReturnSignalsInEmissionOrder();
    void publicSignalBatchPlansDeduplicateSignalsInEmissionOrder();
    void emitterDispatchesChangeSignalsInProjectionOrder();
};

namespace {
void comparePublicSignals(const std::vector<KiriView::VideoDocumentPublicSignal> &actual,
    const std::vector<KiriView::VideoDocumentPublicSignal> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

KiriView::VideoDocumentPublicSignalOperations recordingOperations(QStringList &events)
{
    KiriView::VideoDocumentPublicSignalOperations operations;
    operations.sourceUrlChanged = [&events]() { events.append(QStringLiteral("sourceUrl")); };
    operations.statusChanged = [&events]() { events.append(QStringLiteral("status")); };
    operations.errorStringChanged = [&events]() { events.append(QStringLiteral("errorString")); };
    operations.windowTitleFileNameChanged
        = [&events]() { events.append(QStringLiteral("windowTitleFileName")); };
    operations.durationChanged = [&events]() { events.append(QStringLiteral("duration")); };
    operations.positionChanged = [&events]() { events.append(QStringLiteral("position")); };
    operations.playingChanged = [&events]() { events.append(QStringLiteral("playing")); };
    operations.seekableChanged = [&events]() { events.append(QStringLiteral("seekable")); };
    operations.hasVideoChanged = [&events]() { events.append(QStringLiteral("hasVideo")); };
    operations.hasAudioChanged = [&events]() { events.append(QStringLiteral("hasAudio")); };
    operations.videoSizeChanged = [&events]() { events.append(QStringLiteral("videoSize")); };
    operations.zoomPercentKnownChanged
        = [&events]() { events.append(QStringLiteral("zoomPercentKnown")); };
    operations.zoomPercentChanged = [&events]() { events.append(QStringLiteral("zoomPercent")); };
    operations.videoOutputChanged = [&events]() { events.append(QStringLiteral("videoOutput")); };
    return operations;
}
}

void TestVideoDocumentPublicSignals::publicSignalPlansReturnSignalsInEmissionOrder()
{
    using Change = KiriView::VideoDocumentChange;
    using Signal = KiriView::VideoDocumentPublicSignal;

    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::SourceUrl), { Signal::SourceUrl });
    comparePublicSignals(KiriView::videoDocumentPublicSignals(Change::Status), { Signal::Status });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::ErrorString), { Signal::ErrorString });
    comparePublicSignals(KiriView::videoDocumentPublicSignals(Change::WindowTitleFileName),
        { Signal::WindowTitleFileName });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::Duration), { Signal::Duration });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::Position), { Signal::Position });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::Playing), { Signal::Playing });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::Seekable), { Signal::Seekable });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::HasVideo), { Signal::HasVideo });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::HasAudio), { Signal::HasAudio });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::VideoSize), { Signal::VideoSize });
    comparePublicSignals(KiriView::videoDocumentPublicSignals(Change::ZoomPercentKnown),
        { Signal::ZoomPercentKnown });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::ZoomPercent), { Signal::ZoomPercent });
    comparePublicSignals(
        KiriView::videoDocumentPublicSignals(Change::VideoOutput), { Signal::VideoOutput });
}

void TestVideoDocumentPublicSignals::publicSignalBatchPlansDeduplicateSignalsInEmissionOrder()
{
    using Change = KiriView::VideoDocumentChange;
    using Signal = KiriView::VideoDocumentPublicSignal;

    comparePublicSignals(KiriView::videoDocumentPublicSignalsForChanges(
                             { Change::Status, Change::Duration, Change::Status, Change::Playing }),
        { Signal::Status, Signal::Duration, Signal::Playing });
}

void TestVideoDocumentPublicSignals::emitterDispatchesChangeSignalsInProjectionOrder()
{
    QStringList events;
    const KiriView::VideoDocumentPublicSignalEmitter emitter(recordingOperations(events));

    emitter.emitChanges({ KiriView::VideoDocumentChange::Position,
        KiriView::VideoDocumentChange::HasVideo, KiriView::VideoDocumentChange::ZoomPercent });
    emitter.emitSignal(KiriView::VideoDocumentPublicSignal::VideoOutput);

    QCOMPARE(events,
        QStringList({
            QStringLiteral("position"),
            QStringLiteral("hasVideo"),
            QStringLiteral("zoomPercent"),
            QStringLiteral("videoOutput"),
        }));
}

QTEST_GUILESS_MAIN(TestVideoDocumentPublicSignals)

#include "test_videodocumentpublicsignals.moc"
