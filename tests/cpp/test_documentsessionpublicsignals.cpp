// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/documentsessionpublicsignals.h"

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
void comparePublicSignals(const std::vector<kiriview::DocumentSessionPublicSignal> &actual,
    const std::vector<kiriview::DocumentSessionPublicSignal> &expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        QCOMPARE(actual.at(index), expected.at(index));
    }
}

kiriview::DocumentSessionPublicSignalOperations recordingOperations(QStringList &events)
{
    kiriview::DocumentSessionPublicSignalOperations operations;
    operations.sourceUrlChanged = [&events]() { events.append(QStringLiteral("sourceUrl")); };
    operations.publicProjectionRevisionChanged
        = [&events]() { events.append(QStringLiteral("publicProjectionRevision")); };
    operations.documentKindChanged = [&events]() { events.append(QStringLiteral("documentKind")); };
    operations.errorStringChanged = [&events]() { events.append(QStringLiteral("errorString")); };
    operations.windowTitleSubjectChanged
        = [&events]() { events.append(QStringLiteral("windowTitleSubject")); };
    operations.activeZoomReadoutChanged
        = [&events]() { events.append(QStringLiteral("activeZoomReadout")); };
    operations.activeMediaReadinessChanged
        = [&events]() { events.append(QStringLiteral("activeMediaReadiness")); };
    operations.displayedMediaOpenWithAvailabilityChanged
        = [&events]() { events.append(QStringLiteral("displayedMediaOpenWithAvailability")); };
    operations.displayedFileDeletionAvailabilityChanged
        = [&events]() { events.append(QStringLiteral("displayedFileDeletionAvailability")); };
    operations.fileDeletionInProgressChanged
        = [&events]() { events.append(QStringLiteral("fileDeletionInProgress")); };
    operations.activeNavigationChanged
        = [&events]() { events.append(QStringLiteral("activeNavigation")); };
    operations.activeNavigationRevealIntentChanged
        = [&events]() { events.append(QStringLiteral("activeNavigationRevealIntent")); };
    operations.activeNavigationRevealDirectionChanged
        = [&events]() { events.append(QStringLiteral("activeNavigationRevealDirection")); };
    return operations;
}
}

void TestDocumentSessionPublicSignals::publicSignalPlansReturnSignalsInEmissionOrder()
{
    using Change = kiriview::DocumentSessionChange;
    using Signal = kiriview::DocumentSessionPublicSignal;

    comparePublicSignals(kiriview::documentSessionPublicSignals(Change::PublicProjectionRevision),
        { Signal::PublicProjectionRevision });
    comparePublicSignals(
        kiriview::documentSessionPublicSignals(Change::SourceUrl), { Signal::SourceUrl });
    comparePublicSignals(
        kiriview::documentSessionPublicSignals(Change::DocumentKind), { Signal::DocumentKind });
    comparePublicSignals(
        kiriview::documentSessionPublicSignals(Change::ErrorString), { Signal::ErrorString });
    comparePublicSignals(kiriview::documentSessionPublicSignals(Change::WindowTitleSubject),
        { Signal::WindowTitleSubject });
    comparePublicSignals(kiriview::documentSessionPublicSignals(Change::ActiveZoomReadout),
        { Signal::ActiveZoomReadout });
    comparePublicSignals(kiriview::documentSessionPublicSignals(Change::ActiveMediaReadiness),
        { Signal::ActiveMediaReadiness });
    comparePublicSignals(kiriview::documentSessionPublicSignals(Change::OpenWithAvailability),
        { Signal::DisplayedMediaOpenWithAvailability });
    comparePublicSignals(kiriview::documentSessionPublicSignals(Change::FileDeletionAvailability),
        { Signal::DisplayedFileDeletionAvailability });
    comparePublicSignals(kiriview::documentSessionPublicSignals(Change::FileDeletionInProgress),
        { Signal::FileDeletionInProgress });
    comparePublicSignals(kiriview::documentSessionPublicSignals(Change::ActiveNavigation),
        { Signal::ActiveNavigation });
    comparePublicSignals(
        kiriview::documentSessionPublicSignals(Change::ActiveNavigationRevealIntent),
        { Signal::ActiveNavigationRevealIntent });
    comparePublicSignals(
        kiriview::documentSessionPublicSignals(Change::ActiveNavigationRevealDirection),
        { Signal::ActiveNavigationRevealDirection });
}

void TestDocumentSessionPublicSignals::publicSignalBatchPlansDeduplicateSignalsInEmissionOrder()
{
    using Change = kiriview::DocumentSessionChange;
    using Signal = kiriview::DocumentSessionPublicSignal;

    comparePublicSignals(
        kiriview::documentSessionPublicSignalsForChanges({ Change::DocumentKind,
            Change::ErrorString, Change::DocumentKind, Change::PublicProjectionRevision,
            Change::ActiveNavigation, Change::ActiveNavigationRevealIntent,
            Change::ActiveNavigationRevealDirection, Change::ActiveNavigationRevealDirection }),
        { Signal::DocumentKind, Signal::ErrorString, Signal::PublicProjectionRevision,
            Signal::ActiveNavigation, Signal::ActiveNavigationRevealIntent,
            Signal::ActiveNavigationRevealDirection });
}

void TestDocumentSessionPublicSignals::emitterDispatchesChangeSignalsInProjectionOrder()
{
    QStringList events;
    const kiriview::DocumentSessionPublicSignalEmitter emitter(recordingOperations(events));

    emitter.emitChanges({ kiriview::DocumentSessionChange::PublicProjectionRevision,
        kiriview::DocumentSessionChange::OpenWithAvailability,
        kiriview::DocumentSessionChange::FileDeletionAvailability,
        kiriview::DocumentSessionChange::FileDeletionInProgress,
        kiriview::DocumentSessionChange::ActiveNavigationRevealIntent,
        kiriview::DocumentSessionChange::ActiveNavigationRevealDirection });
    emitter.emitSignal(kiriview::DocumentSessionPublicSignal::SourceUrl);

    QCOMPARE(events,
        QStringList({
            QStringLiteral("publicProjectionRevision"),
            QStringLiteral("displayedMediaOpenWithAvailability"),
            QStringLiteral("displayedFileDeletionAvailability"),
            QStringLiteral("fileDeletionInProgress"),
            QStringLiteral("activeNavigationRevealIntent"),
            QStringLiteral("activeNavigationRevealDirection"),
            QStringLiteral("sourceUrl"),
        }));
}

QTEST_GUILESS_MAIN(TestDocumentSessionPublicSignals)

#include "test_documentsessionpublicsignals.moc"
