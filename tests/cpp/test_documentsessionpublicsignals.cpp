// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionpublicsignals.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <vector>

class TestDocumentSessionPublicSignals : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void publicSignalPlansReturnSignalsInEmissionOrder();
    void publicSignalBatchPlansDeduplicateSignalsInEmissionOrder();
    void emitterDispatchesChangeSignalsInProjectionOrder();
};

namespace {
void comparePublicSignals(const std::vector<KiriView::DocumentSessionPublicSignal> &actual,
    const std::vector<KiriView::DocumentSessionPublicSignal> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

KiriView::DocumentSessionPublicSignalOperations recordingOperations(QStringList &events)
{
    KiriView::DocumentSessionPublicSignalOperations operations;
    operations.sourceUrlChanged = [&events]() { events.append(QStringLiteral("sourceUrl")); };
    operations.documentKindChanged = [&events]() { events.append(QStringLiteral("documentKind")); };
    operations.errorStringChanged = [&events]() { events.append(QStringLiteral("errorString")); };
    operations.windowTitleSubjectChanged
        = [&events]() { events.append(QStringLiteral("windowTitleSubject")); };
    operations.activeZoomReadoutChanged
        = [&events]() { events.append(QStringLiteral("activeZoomReadout")); };
    operations.displayedFileDeletionAvailabilityChanged
        = [&events]() { events.append(QStringLiteral("displayedFileDeletionAvailability")); };
    operations.fileDeletionInProgressChanged
        = [&events]() { events.append(QStringLiteral("fileDeletionInProgress")); };
    operations.activeNavigationChanged
        = [&events]() { events.append(QStringLiteral("activeNavigation")); };
    return operations;
}
}

void TestDocumentSessionPublicSignals::publicSignalPlansReturnSignalsInEmissionOrder()
{
    using Change = KiriView::DocumentSessionChange;
    using Signal = KiriView::DocumentSessionPublicSignal;

    comparePublicSignals(
        KiriView::documentSessionPublicSignals(Change::SourceUrl), { Signal::SourceUrl });
    comparePublicSignals(
        KiriView::documentSessionPublicSignals(Change::DocumentKind), { Signal::DocumentKind });
    comparePublicSignals(
        KiriView::documentSessionPublicSignals(Change::ErrorString), { Signal::ErrorString });
    comparePublicSignals(KiriView::documentSessionPublicSignals(Change::WindowTitleSubject),
        { Signal::WindowTitleSubject });
    comparePublicSignals(KiriView::documentSessionPublicSignals(Change::ActiveZoomReadout),
        { Signal::ActiveZoomReadout });
    comparePublicSignals(KiriView::documentSessionPublicSignals(Change::FileDeletionAvailability),
        { Signal::DisplayedFileDeletionAvailability });
    comparePublicSignals(KiriView::documentSessionPublicSignals(Change::FileDeletionInProgress),
        { Signal::FileDeletionInProgress });
    comparePublicSignals(KiriView::documentSessionPublicSignals(Change::ActiveNavigation),
        { Signal::ActiveNavigation });
}

void TestDocumentSessionPublicSignals::publicSignalBatchPlansDeduplicateSignalsInEmissionOrder()
{
    using Change = KiriView::DocumentSessionChange;
    using Signal = KiriView::DocumentSessionPublicSignal;

    comparePublicSignals(KiriView::documentSessionPublicSignalsForChanges({ Change::DocumentKind,
                             Change::ErrorString, Change::DocumentKind, Change::ActiveNavigation }),
        { Signal::DocumentKind, Signal::ErrorString, Signal::ActiveNavigation });
}

void TestDocumentSessionPublicSignals::emitterDispatchesChangeSignalsInProjectionOrder()
{
    QStringList events;
    const KiriView::DocumentSessionPublicSignalEmitter emitter(recordingOperations(events));

    emitter.emitChanges({ KiriView::DocumentSessionChange::FileDeletionAvailability,
        KiriView::DocumentSessionChange::FileDeletionInProgress });
    emitter.emitSignal(KiriView::DocumentSessionPublicSignal::SourceUrl);

    QCOMPARE(events,
        QStringList({
            QStringLiteral("displayedFileDeletionAvailability"),
            QStringLiteral("fileDeletionInProgress"),
            QStringLiteral("sourceUrl"),
        }));
}

QTEST_GUILESS_MAIN(TestDocumentSessionPublicSignals)

#include "test_documentsessionpublicsignals.moc"
