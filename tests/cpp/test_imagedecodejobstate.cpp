// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/imagedecodejobstate.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <limits>
#include <variant>

namespace {
kiriview::ImageDecodeRequest request(int index)
{
    return kiriview::ImageDecodeRequest::fromUrl(
        static_cast<quint64>(index), kiriview::TestSupport::indexedImageUrl(index));
}

void compareNoOperation(const kiriview::ImageDecodeJobRuntimePlan &plan)
{
    QVERIFY(std::holds_alternative<kiriview::NoImageDecodeJobOperation>(plan.operation));
    QVERIFY(!plan.hasOperation());
}

template <typename Operation>
void compareOperation(const kiriview::ImageDecodeJobRuntimePlan &plan, quint64 requestId)
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
    kiriview::ImageDecodeJobState state;
    const kiriview::ImageDecodeJobTicket ticket = state.start(request(1));

    QVERIFY(state.hasActiveRequest());
    compareOperation<kiriview::StartImageDecodeOperation>(state.acceptLoadedData(ticket), 1);
    compareNoOperation(state.acceptLoadError(ticket));

    compareOperation<kiriview::DeliverImageDecodeResultOperation>(
        state.acceptDecodeResult(ticket), 1);
    QVERIFY(!state.hasActiveRequest());
    compareNoOperation(state.acceptDecodeResult(ticket));
}

void TestImageDecodeJobState::loadPhaseClaimsOnlyLoadErrors()
{
    kiriview::ImageDecodeJobState state;
    const kiriview::ImageDecodeJobTicket ticket = state.start(request(8));

    compareNoOperation(state.acceptDecodeResult(ticket));
    compareOperation<kiriview::DeliverImageLoadErrorOperation>(state.acceptLoadError(ticket), 8);
    QVERIFY(!state.hasActiveRequest());
    compareNoOperation(state.acceptLoadError(ticket));
}

void TestImageDecodeJobState::restartedSameRequestRejectsStaleTicket()
{
    kiriview::ImageDecodeJobState state;
    const kiriview::ImageDecodeRequest decodeRequest = request(2);
    const kiriview::ImageDecodeJobTicket staleTicket = state.start(decodeRequest);
    const kiriview::ImageDecodeJobTicket currentTicket = state.start(decodeRequest);

    compareNoOperation(state.acceptLoadedData(staleTicket));
    compareNoOperation(state.acceptLoadError(staleTicket));
    compareOperation<kiriview::StartImageDecodeOperation>(state.acceptLoadedData(currentTicket), 2);
}

void TestImageDecodeJobState::restartedDuringDecodeRejectsStaleDecodeResult()
{
    kiriview::ImageDecodeJobState state;
    const kiriview::ImageDecodeJobTicket staleTicket = state.start(request(4));
    compareOperation<kiriview::StartImageDecodeOperation>(state.acceptLoadedData(staleTicket), 4);

    const kiriview::ImageDecodeJobTicket currentTicket = state.start(request(5));

    compareNoOperation(state.acceptDecodeResult(staleTicket));
    compareOperation<kiriview::StartImageDecodeOperation>(state.acceptLoadedData(currentTicket), 5);
    compareOperation<kiriview::DeliverImageDecodeResultOperation>(
        state.acceptDecodeResult(currentTicket), 5);
}

void TestImageDecodeJobState::cancelInvalidatesActiveTicket()
{
    kiriview::ImageDecodeJobState state;
    const kiriview::ImageDecodeJobTicket ticket = state.start(request(6));

    state.cancel();

    QVERIFY(!state.hasActiveRequest());
    compareNoOperation(state.acceptLoadedData(ticket));
    compareNoOperation(state.acceptLoadError(ticket));
}

void TestImageDecodeJobState::operationIdsStayNonZeroAfterWrap()
{
    kiriview::ImageDecodeJobState state(std::numeric_limits<quint64>::max());

    const kiriview::ImageDecodeJobTicket ticket = state.start(request(7));

    QVERIFY(ticket.operationId != 0);
    compareOperation<kiriview::StartImageDecodeOperation>(state.acceptLoadedData(ticket), 7);
}

QTEST_GUILESS_MAIN(TestImageDecodeJobState)

#include "test_imagedecodejobstate.moc"
