// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportcommandstate.h"

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QTest>

class TestImageViewportCommandState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void commandsPublishMonotonicRevisions();
    void newerCommandSupersedesOlderAcknowledgement();
    void userObservationPromotesCanonicalFrame();
    void commandAcknowledgementRejectsImpossibleRevision();
};

void TestImageViewportCommandState::commandsPublishMonotonicRevisions()
{
    KiriView::ImageViewportCommandState state;
    state.setGeometry(QSizeF(200.0, 160.0), QSizeF(1000.0, 800.0));

    const KiriView::ImageViewportCommand first = state.requestContentPosition(QPointF(10.0, 20.0));
    const KiriView::ImageViewportCommand second = state.requestContentPosition(QPointF(30.0, 40.0));

    QCOMPARE(first.revision, 1);
    QCOMPARE(second.revision, 2);
    QCOMPARE(state.projection().commandRevision, 2);
    QCOMPARE(state.projection().requestedContentPosition, QPointF(30.0, 40.0));
    QCOMPARE(state.projection().status, KiriView::ImageViewportCommandStatus::Pending);
}

void TestImageViewportCommandState::newerCommandSupersedesOlderAcknowledgement()
{
    KiriView::ImageViewportCommandState state;
    state.setGeometry(QSizeF(200.0, 160.0), QSizeF(1000.0, 800.0));

    const KiriView::ImageViewportCommand stale = state.requestContentPosition(QPointF(10.0, 20.0));
    const KiriView::ImageViewportCommand latest = state.requestContentPosition(QPointF(30.0, 40.0));

    QVERIFY(!state.acknowledgeCommand(stale.revision, QPointF(10.0, 20.0)));
    QCOMPARE(state.projection().status, KiriView::ImageViewportCommandStatus::Superseded);
    QCOMPARE(state.projection().commandRevision, latest.revision);
    QCOMPARE(state.projection().frame.contentPosition, QPointF(30.0, 40.0));
}

void TestImageViewportCommandState::userObservationPromotesCanonicalFrame()
{
    KiriView::ImageViewportCommandState state;
    state.setGeometry(QSizeF(200.0, 160.0), QSizeF(1000.0, 800.0));
    state.requestContentPosition(QPointF(30.0, 40.0));

    QVERIFY(state.observeContentPosition(
        QPointF(70.0, 80.0), KiriView::ImageViewportObservationOrigin::User));

    QCOMPARE(state.projection().observationRevision, 1);
    QCOMPARE(state.projection().observationOrigin, KiriView::ImageViewportObservationOrigin::User);
    QCOMPARE(state.projection().status, KiriView::ImageViewportCommandStatus::Settled);
    QCOMPARE(state.projection().frame.contentPosition, QPointF(70.0, 80.0));
    QCOMPARE(state.projection().frame.visibleItemRect, QRectF(70.0, 80.0, 200.0, 160.0));
}

void TestImageViewportCommandState::commandAcknowledgementRejectsImpossibleRevision()
{
    KiriView::ImageViewportCommandState state;
    state.setGeometry(QSizeF(200.0, 160.0), QSizeF(1000.0, 800.0));
    state.requestContentPosition(QPointF(30.0, 40.0));

    QVERIFY(!state.acknowledgeCommand(99, QPointF(30.0, 40.0)));

    QCOMPARE(state.projection().status, KiriView::ImageViewportCommandStatus::Rejected);
    QCOMPARE(state.projection().appliedCommandRevision, 0);
    QCOMPARE(state.projection().frame.contentPosition, QPointF(30.0, 40.0));
}

QTEST_GUILESS_MAIN(TestImageViewportCommandState)

#include "test_imageviewportcommandstate.moc"
