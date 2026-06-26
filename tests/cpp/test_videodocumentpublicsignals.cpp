// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/videodocumentpublicsignals.h"

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
void comparePublicSignals(const std::vector<kiriview::VideoDocumentPublicSignal>& actual,
    const std::vector<kiriview::VideoDocumentPublicSignal>& expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

kiriview::VideoDocumentPublicSignalOperations recordingOperations(QStringList& events)
{
    kiriview::VideoDocumentPublicSignalOperations operations;
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
    operations.mutedChanged = [&events]() { events.append(QStringLiteral("muted")); };
    operations.videoOutputChanged = [&events]() { events.append(QStringLiteral("videoOutput")); };
    return operations;
}
}

void TestVideoDocumentPublicSignals::publicSignalPlansReturnSignalsInEmissionOrder()
{
    using Change = kiriview::VideoDocumentChange;
    using Signal = kiriview::VideoDocumentPublicSignal;

    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::SourceUrl), { Signal::SourceUrl });
    comparePublicSignals(kiriview::videoDocumentPublicSignals(Change::Status), { Signal::Status });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::ErrorString), { Signal::ErrorString });
    comparePublicSignals(kiriview::videoDocumentPublicSignals(Change::WindowTitleFileName),
        { Signal::WindowTitleFileName });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::Duration), { Signal::Duration });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::Position), { Signal::Position });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::Playing), { Signal::Playing });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::Seekable), { Signal::Seekable });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::HasVideo), { Signal::HasVideo });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::HasAudio), { Signal::HasAudio });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::VideoSize), { Signal::VideoSize });
    comparePublicSignals(kiriview::videoDocumentPublicSignals(Change::ZoomPercentKnown),
        { Signal::ZoomPercentKnown });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::ZoomPercent), { Signal::ZoomPercent });
    comparePublicSignals(kiriview::videoDocumentPublicSignals(Change::Muted), { Signal::Muted });
    comparePublicSignals(
        kiriview::videoDocumentPublicSignals(Change::VideoOutput), { Signal::VideoOutput });
}

void TestVideoDocumentPublicSignals::publicSignalBatchPlansDeduplicateSignalsInEmissionOrder()
{
    using Change = kiriview::VideoDocumentChange;
    using Signal = kiriview::VideoDocumentPublicSignal;

    comparePublicSignals(kiriview::videoDocumentPublicSignalsForChanges(
                             { Change::Status, Change::Duration, Change::Status, Change::Playing }),
        { Signal::Status, Signal::Duration, Signal::Playing });
}

void TestVideoDocumentPublicSignals::emitterDispatchesChangeSignalsInProjectionOrder()
{
    QStringList events;
    const kiriview::VideoDocumentPublicSignalEmitter emitter(recordingOperations(events));

    emitter.emitChanges(
        { kiriview::VideoDocumentChange::Position, kiriview::VideoDocumentChange::HasVideo,
            kiriview::VideoDocumentChange::ZoomPercent, kiriview::VideoDocumentChange::Muted });
    emitter.emitSignal(kiriview::VideoDocumentPublicSignal::VideoOutput);

    QCOMPARE(events,
        QStringList({
            QStringLiteral("position"),
            QStringLiteral("hasVideo"),
            QStringLiteral("zoomPercent"),
            QStringLiteral("muted"),
            QStringLiteral("videoOutput"),
        }));
}

QTEST_GUILESS_MAIN(TestVideoDocumentPublicSignals)

#include "test_videodocumentpublicsignals.moc"
