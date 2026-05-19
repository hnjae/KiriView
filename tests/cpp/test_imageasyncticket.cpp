// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageasyncticket.h"

#include <QObject>
#include <QTest>
#include <limits>

class TestImageAsyncTicket : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ticketsAreInactiveUntilAdvanced();
    void nextPublishesOnlyCurrentNonZeroTicket();
    void invalidateRejectsPreviousTicket();
    void ticketsStayNonZeroAfterWrap();
};

void TestImageAsyncTicket::ticketsAreInactiveUntilAdvanced()
{
    KiriView::ImageAsyncTicket ticket;

    QCOMPARE(ticket.current(), quint64(0));
    QVERIFY(!ticket.accepts(0));
}

void TestImageAsyncTicket::nextPublishesOnlyCurrentNonZeroTicket()
{
    KiriView::ImageAsyncTicket ticket;

    const quint64 first = ticket.next();
    const quint64 second = ticket.next();

    QCOMPARE(first, quint64(1));
    QCOMPARE(second, quint64(2));
    QVERIFY(!ticket.accepts(0));
    QVERIFY(!ticket.accepts(first));
    QVERIFY(ticket.accepts(second));
}

void TestImageAsyncTicket::invalidateRejectsPreviousTicket()
{
    KiriView::ImageAsyncTicket ticket;
    const quint64 stale = ticket.next();

    ticket.invalidate();

    QVERIFY(!ticket.accepts(stale));
    QVERIFY(ticket.accepts(ticket.current()));
}

void TestImageAsyncTicket::ticketsStayNonZeroAfterWrap()
{
    KiriView::ImageAsyncTicket ticket(std::numeric_limits<quint64>::max());

    QCOMPARE(ticket.next(), quint64(1));
    QVERIFY(ticket.accepts(1));
    QVERIFY(!ticket.accepts(0));
}

QTEST_GUILESS_MAIN(TestImageAsyncTicket)

#include "test_imageasyncticket.moc"
