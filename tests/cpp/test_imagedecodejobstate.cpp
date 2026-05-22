// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedecodejobstate.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <limits>
#include <variant>

namespace {
KiriView::ImageDecodeRequest request(int index)
{
    return KiriView::ImageDecodeRequest::fromUrl(
        static_cast<quint64>(index), KiriView::TestSupport::indexedImageUrl(index));
}

void compareNoOperation(const KiriView::ImageDecodeJobRuntimePlan &plan)
{
    QVERIFY(std::holds_alternative<KiriView::NoImageDecodeJobOperation>(plan.operation));
    QVERIFY(!plan.hasOperation());
}

template <typename Operation>
void compareOperation(const KiriView::ImageDecodeJobRuntimePlan &plan, quint64 requestId)
{
    const auto *operation = std::get_if<Operation>(&plan.operation);
    QVERIFY(operation != nullptr);
    QVERIFY(plan.hasOperation());
    QCOMPARE(operation->request.id(), requestId);
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
    compareOperation<KiriView::StartImageDecodeOperation>(state.acceptLoadedData(ticket), 1);
    compareNoOperation(state.acceptLoadError(ticket));

    compareOperation<KiriView::DeliverImageDecodeResultOperation>(
        state.acceptDecodeResult(ticket), 1);
    QVERIFY(!state.hasActiveRequest());
    compareNoOperation(state.acceptDecodeResult(ticket));
}

void TestImageDecodeJobState::loadPhaseClaimsOnlyLoadErrors()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeJobTicket ticket = state.start(request(8));

    compareNoOperation(state.acceptDecodeResult(ticket));
    compareOperation<KiriView::DeliverImageLoadErrorOperation>(state.acceptLoadError(ticket), 8);
    QVERIFY(!state.hasActiveRequest());
    compareNoOperation(state.acceptLoadError(ticket));
}

void TestImageDecodeJobState::restartedSameRequestRejectsStaleTicket()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeRequest decodeRequest = request(2);
    const KiriView::ImageDecodeJobTicket staleTicket = state.start(decodeRequest);
    const KiriView::ImageDecodeJobTicket currentTicket = state.start(decodeRequest);

    compareNoOperation(state.acceptLoadedData(staleTicket));
    compareNoOperation(state.acceptLoadError(staleTicket));
    compareOperation<KiriView::StartImageDecodeOperation>(state.acceptLoadedData(currentTicket), 2);
}

void TestImageDecodeJobState::restartedDuringDecodeRejectsStaleDecodeResult()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeJobTicket staleTicket = state.start(request(4));
    compareOperation<KiriView::StartImageDecodeOperation>(state.acceptLoadedData(staleTicket), 4);

    const KiriView::ImageDecodeJobTicket currentTicket = state.start(request(5));

    compareNoOperation(state.acceptDecodeResult(staleTicket));
    compareOperation<KiriView::StartImageDecodeOperation>(state.acceptLoadedData(currentTicket), 5);
    compareOperation<KiriView::DeliverImageDecodeResultOperation>(
        state.acceptDecodeResult(currentTicket), 5);
}

void TestImageDecodeJobState::cancelInvalidatesActiveTicket()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeJobTicket ticket = state.start(request(6));

    state.cancel();

    QVERIFY(!state.hasActiveRequest());
    compareNoOperation(state.acceptLoadedData(ticket));
    compareNoOperation(state.acceptLoadError(ticket));
}

void TestImageDecodeJobState::operationIdsStayNonZeroAfterWrap()
{
    KiriView::ImageDecodeJobState state(std::numeric_limits<quint64>::max());

    const KiriView::ImageDecodeJobTicket ticket = state.start(request(7));

    QVERIFY(ticket.operationId != 0);
    compareOperation<KiriView::StartImageDecodeOperation>(state.acceptLoadedData(ticket), 7);
}

QTEST_GUILESS_MAIN(TestImageDecodeJobState)

#include "test_imagedecodejobstate.moc"
