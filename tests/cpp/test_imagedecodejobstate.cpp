// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedecodejobstate.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <limits>

namespace {
KiriView::ImageDecodeRequest request(int index)
{
    return KiriView::ImageDecodeRequest::fromUrl(
        static_cast<quint64>(index), KiriView::TestSupport::indexedImageUrl(index));
}

void comparePlan(const KiriView::ImageDecodeJobRuntimePlan &plan,
    KiriView::ImageDecodeJobRuntimeOperation operation, quint64 requestId = 0)
{
    QCOMPARE(plan.operation, operation);
    QCOMPARE(plan.hasOperation(), operation != KiriView::ImageDecodeJobRuntimeOperation::None);
    if (operation != KiriView::ImageDecodeJobRuntimeOperation::None) {
        QCOMPARE(plan.request.id(), requestId);
    }
}
}

class TestImageDecodeJobState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void decodePhaseClaimsOnlyDecodeResults();
    void loadPhaseClaimsOnlyLoadErrors();
    void restartedSameRequestRejectsStaleTicket();
    void restartedDuringDecodeRejectsStaleDecodeResult();
    void cancelInvalidatesActiveTicket();
    void operationIdsStayNonZeroAfterWrap();
};

void TestImageDecodeJobState::decodePhaseClaimsOnlyDecodeResults()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeJobTicket ticket = state.start(request(1));

    QVERIFY(state.hasActiveRequest());
    comparePlan(
        state.acceptLoadedData(ticket), KiriView::ImageDecodeJobRuntimeOperation::StartDecode, 1);
    comparePlan(state.acceptLoadError(ticket), KiriView::ImageDecodeJobRuntimeOperation::None);

    comparePlan(state.acceptDecodeResult(ticket),
        KiriView::ImageDecodeJobRuntimeOperation::DeliverDecodeResult, 1);
    QVERIFY(!state.hasActiveRequest());
    comparePlan(state.acceptDecodeResult(ticket), KiriView::ImageDecodeJobRuntimeOperation::None);
}

void TestImageDecodeJobState::loadPhaseClaimsOnlyLoadErrors()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeJobTicket ticket = state.start(request(8));

    comparePlan(state.acceptDecodeResult(ticket), KiriView::ImageDecodeJobRuntimeOperation::None);
    comparePlan(state.acceptLoadError(ticket),
        KiriView::ImageDecodeJobRuntimeOperation::DeliverLoadError, 8);
    QVERIFY(!state.hasActiveRequest());
    comparePlan(state.acceptLoadError(ticket), KiriView::ImageDecodeJobRuntimeOperation::None);
}

void TestImageDecodeJobState::restartedSameRequestRejectsStaleTicket()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeRequest decodeRequest = request(2);
    const KiriView::ImageDecodeJobTicket staleTicket = state.start(decodeRequest);
    const KiriView::ImageDecodeJobTicket currentTicket = state.start(decodeRequest);

    comparePlan(
        state.acceptLoadedData(staleTicket), KiriView::ImageDecodeJobRuntimeOperation::None);
    comparePlan(state.acceptLoadError(staleTicket), KiriView::ImageDecodeJobRuntimeOperation::None);
    comparePlan(state.acceptLoadedData(currentTicket),
        KiriView::ImageDecodeJobRuntimeOperation::StartDecode, 2);
}

void TestImageDecodeJobState::restartedDuringDecodeRejectsStaleDecodeResult()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeJobTicket staleTicket = state.start(request(4));
    comparePlan(state.acceptLoadedData(staleTicket),
        KiriView::ImageDecodeJobRuntimeOperation::StartDecode, 4);

    const KiriView::ImageDecodeJobTicket currentTicket = state.start(request(5));

    comparePlan(
        state.acceptDecodeResult(staleTicket), KiriView::ImageDecodeJobRuntimeOperation::None);
    comparePlan(state.acceptLoadedData(currentTicket),
        KiriView::ImageDecodeJobRuntimeOperation::StartDecode, 5);
    comparePlan(state.acceptDecodeResult(currentTicket),
        KiriView::ImageDecodeJobRuntimeOperation::DeliverDecodeResult, 5);
}

void TestImageDecodeJobState::cancelInvalidatesActiveTicket()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeJobTicket ticket = state.start(request(6));

    state.cancel();

    QVERIFY(!state.hasActiveRequest());
    comparePlan(state.acceptLoadedData(ticket), KiriView::ImageDecodeJobRuntimeOperation::None);
    comparePlan(state.acceptLoadError(ticket), KiriView::ImageDecodeJobRuntimeOperation::None);
}

void TestImageDecodeJobState::operationIdsStayNonZeroAfterWrap()
{
    KiriView::ImageDecodeJobState state(std::numeric_limits<quint64>::max());

    const KiriView::ImageDecodeJobTicket ticket = state.start(request(7));

    QVERIFY(ticket.operationId != 0);
    comparePlan(
        state.acceptLoadedData(ticket), KiriView::ImageDecodeJobRuntimeOperation::StartDecode, 7);
}

QTEST_GUILESS_MAIN(TestImageDecodeJobState)

#include "test_imagedecodejobstate.moc"
