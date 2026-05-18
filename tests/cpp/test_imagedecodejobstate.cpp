// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedecodejobstate.h"

#include "image_test_support.h"

#include <QObject>
#include <QTest>
#include <optional>

namespace {
KiriView::ImageDecodeRequest request(int index)
{
    return KiriView::ImageDecodeRequest::fromUrl(
        static_cast<quint64>(index), KiriView::TestSupport::indexedImageUrl(index));
}
}

class TestImageDecodeJobState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void decodePhaseClaimsOnlyDecodeResults();
    void restartedSameRequestRejectsStaleTicket();
    void cancelInvalidatesActiveTicket();
};

void TestImageDecodeJobState::decodePhaseClaimsOnlyDecodeResults()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeJobTicket ticket = state.start(request(1));

    QVERIFY(state.hasActiveRequest());
    QVERIFY(state.beginDecode(ticket).has_value());
    QVERIFY(!state.claimLoadError(ticket).has_value());

    std::optional<KiriView::ImageDecodeRequest> claimed = state.claimDecodeResult(ticket);
    QVERIFY(claimed.has_value());
    QCOMPARE(claimed->id(), quint64(1));
    QVERIFY(!state.hasActiveRequest());
    QVERIFY(!state.claimDecodeResult(ticket).has_value());
}

void TestImageDecodeJobState::restartedSameRequestRejectsStaleTicket()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeRequest decodeRequest = request(2);
    const KiriView::ImageDecodeJobTicket staleTicket = state.start(decodeRequest);
    const KiriView::ImageDecodeJobTicket currentTicket = state.start(decodeRequest);

    QVERIFY(!state.beginDecode(staleTicket).has_value());
    QVERIFY(!state.claimLoadError(staleTicket).has_value());
    QVERIFY(state.beginDecode(currentTicket).has_value());
}

void TestImageDecodeJobState::cancelInvalidatesActiveTicket()
{
    KiriView::ImageDecodeJobState state;
    const KiriView::ImageDecodeJobTicket ticket = state.start(request(3));

    state.cancel();

    QVERIFY(!state.hasActiveRequest());
    QVERIFY(!state.beginDecode(ticket).has_value());
    QVERIFY(!state.claimLoadError(ticket).has_value());
}

QTEST_GUILESS_MAIN(TestImageDecodeJobState)

#include "test_imagedecodejobstate.moc"
