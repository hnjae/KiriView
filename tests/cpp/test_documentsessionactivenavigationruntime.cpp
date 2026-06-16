// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionactivenavigationruntime.h"

#include <QObject>
#include <QTest>

#include <vector>

class TestDocumentSessionActiveNavigationRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void acceptedImagePageDispatchStoresPendingRevealAndExecutesPort();
    void rejectedDispatchClearsRevealAndPublishes();
    void numberedDirectMediaDispatchUsesDirectMediaPort();
};

namespace {
kiriview::ActiveNavigationSnapshot activeNavigationSnapshot(int currentNumber, int count)
{
    return kiriview::ActiveNavigationSnapshot {
        true,
        true,
        true,
        currentNumber > 1,
        currentNumber < count,
        currentNumber == 1,
        currentNumber == count,
        currentNumber,
        count,
    };
}
}

void TestDocumentSessionActiveNavigationRuntime::
    acceptedImagePageDispatchStoresPendingRevealAndExecutesPort()
{
    int previousPageCount = 0;
    std::vector<kiriview::ActiveNavigationRevealContext> revealContexts;
    kiriview::DocumentSessionActiveNavigationRuntimePorts ports;
    ports.setRevealContext = [&revealContexts](kiriview::ActiveNavigationRevealContext context) {
        revealContexts.push_back(context);
    };
    ports.openPreviousImageDocumentPage = [&previousPageCount]() { ++previousPageCount; };

    kiriview::DocumentSessionActiveNavigationRuntime runtime(std::move(ports));
    const kiriview::ActiveNavigationRevealContext dispatchContext {
        kiriview::ActiveNavigationRevealIntent::AdjacentNavigation,
        kiriview::ActiveNavigationRevealDirection::Previous,
    };

    const kiriview::ActiveNavigationDispatchOutcome outcome = runtime.dispatch(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, activeNavigationSnapshot(2, 4),
        kiriview::previousActiveNavigationDispatchRequest(), dispatchContext);

    QCOMPARE(outcome, kiriview::ActiveNavigationDispatchOutcome::Dispatch);
    QCOMPARE(previousPageCount, 1);
    QCOMPARE(revealContexts.size(), std::size_t(1));
    QCOMPARE(revealContexts.front().intent, dispatchContext.intent);
    QCOMPARE(revealContexts.front().direction, dispatchContext.direction);

    const kiriview::ActiveNavigationRevealContext pending = runtime.takePendingRevealContext(
        kiriview::ActiveNavigationRevealIntent::ProgrammaticSync);
    QCOMPARE(pending.intent, dispatchContext.intent);
    QCOMPARE(pending.direction, dispatchContext.direction);

    const kiriview::ActiveNavigationRevealContext fallback
        = runtime.takePendingRevealContext(kiriview::ActiveNavigationRevealIntent::LargeJump);
    QCOMPARE(fallback.intent, kiriview::ActiveNavigationRevealIntent::LargeJump);
    QCOMPARE(fallback.direction, kiriview::ActiveNavigationRevealDirection::None);
}

void TestDocumentSessionActiveNavigationRuntime::rejectedDispatchClearsRevealAndPublishes()
{
    int recomputeCount = 0;
    std::vector<kiriview::ActiveNavigationRevealContext> revealContexts;
    kiriview::DocumentSessionActiveNavigationRuntimePorts ports;
    ports.setRevealContext = [&revealContexts](kiriview::ActiveNavigationRevealContext context) {
        revealContexts.push_back(context);
    };
    ports.recomputePublicProjection = [&recomputeCount]() { ++recomputeCount; };

    kiriview::DocumentSessionActiveNavigationRuntime runtime(std::move(ports));
    runtime.setPendingRevealContext(kiriview::ActiveNavigationRevealContext {
        kiriview::ActiveNavigationRevealIntent::AdjacentNavigation,
        kiriview::ActiveNavigationRevealDirection::Next,
    });

    const kiriview::ActiveNavigationDispatchOutcome outcome
        = runtime.dispatch(kiriview::ActiveNavigationSourceKind::None,
            kiriview::ActiveNavigationSnapshot {}, kiriview::nextActiveNavigationDispatchRequest(),
            kiriview::ActiveNavigationRevealContext {
                kiriview::ActiveNavigationRevealIntent::AdjacentNavigation,
                kiriview::ActiveNavigationRevealDirection::Next,
            });

    QCOMPARE(outcome, kiriview::ActiveNavigationDispatchOutcome::NoOp);
    QCOMPARE(recomputeCount, 1);
    QCOMPARE(revealContexts.size(), std::size_t(2));
    QCOMPARE(revealContexts.back().intent, kiriview::ActiveNavigationRevealIntent::None);
    QCOMPARE(revealContexts.back().direction, kiriview::ActiveNavigationRevealDirection::None);

    const kiriview::ActiveNavigationRevealContext fallback = runtime.takePendingRevealContext(
        kiriview::ActiveNavigationRevealIntent::ProgrammaticSync);
    QCOMPARE(fallback.intent, kiriview::ActiveNavigationRevealIntent::ProgrammaticSync);
    QCOMPARE(fallback.direction, kiriview::ActiveNavigationRevealDirection::None);
}

void TestDocumentSessionActiveNavigationRuntime::numberedDirectMediaDispatchUsesDirectMediaPort()
{
    int openedNumber = 0;
    kiriview::DocumentSessionActiveNavigationRuntimePorts ports;
    ports.setRevealContext = [](kiriview::ActiveNavigationRevealContext) { };
    ports.openDirectMediaAtNumber = [&openedNumber](int number) { openedNumber = number; };

    kiriview::DocumentSessionActiveNavigationRuntime runtime(std::move(ports));

    const kiriview::ActiveNavigationDispatchOutcome outcome
        = runtime.dispatch(kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
            activeNavigationSnapshot(2, 5), kiriview::numberedActiveNavigationDispatchRequest(4),
            kiriview::ActiveNavigationRevealContext {
                kiriview::ActiveNavigationRevealIntent::LargeJump,
            });

    QCOMPARE(outcome, kiriview::ActiveNavigationDispatchOutcome::Dispatch);
    QCOMPARE(openedNumber, 4);
}

QTEST_GUILESS_MAIN(TestDocumentSessionActiveNavigationRuntime)

#include "test_documentsessionactivenavigationruntime.moc"
