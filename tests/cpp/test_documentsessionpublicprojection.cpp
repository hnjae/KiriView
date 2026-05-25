// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionpublicprojection.h"

#include <QObject>
#include <QSize>
#include <QTest>

class TestDocumentSessionPublicProjection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptySessionProjectsUnavailableNavigationAndEmptyTitle();
    void directImageProjectsMediaNavigationAndIntrinsicSizeTitle();
    void archiveImageProjectsPageNavigationAndCounterTitle();
    void deletionMasksNavigationDispatchWithoutDroppingTitleCounter();
};

void TestDocumentSessionPublicProjection::emptySessionProjectsUnavailableNavigationAndEmptyTitle()
{
    const KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState({});

    QCOMPARE(projection.sourceKind, KiriView::ActiveNavigationSourceKind::None);
    QCOMPARE(projection.boundaryScope, KiriView::ActiveNavigationBoundaryScope::None);
    QVERIFY(!projection.activeNavigation.available);
    QCOMPARE(projection.windowTitleSubject, QString());
}

void TestDocumentSessionPublicProjection::directImageProjectsMediaNavigationAndIntrinsicSizeTitle()
{
    const KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState(
            KiriView::DocumentSessionPublicProjectionInput {
                KiriView::DocumentSessionKind::Image,
                true,
                false,
                false,
                KiriView::MediaActiveNavigationInput {
                    KiriView::MediaNavigationBoundaryState { false, true, true, false, 1, 3 },
                    true,
                },
                {},
                QStringLiteral("01.png"),
                QSize(640, 480),
                {},
                {},
            });

    QCOMPARE(projection.sourceKind, KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia);
    QCOMPARE(projection.boundaryScope, KiriView::ActiveNavigationBoundaryScope::Media);
    QVERIFY(projection.activeNavigation.known);
    QVERIFY(projection.activeNavigation.canOpenNext);
    QCOMPARE(projection.activeNavigation.currentNumber, 1);
    QCOMPARE(projection.activeNavigation.count, 3);
    QCOMPARE(projection.windowTitleSubject, QStringLiteral("01.png – 640×480"));
}

void TestDocumentSessionPublicProjection::archiveImageProjectsPageNavigationAndCounterTitle()
{
    const KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState(
            KiriView::DocumentSessionPublicProjectionInput {
                KiriView::DocumentSessionKind::Image,
                false,
                true,
                false,
                {},
                KiriView::ImageDocumentActiveNavigationInput { 2, 3, 5 },
                QStringLiteral("book.cbz"),
                QSize(640, 480),
                {},
                {},
            });

    QCOMPARE(projection.sourceKind, KiriView::ActiveNavigationSourceKind::ImageDocumentPages);
    QCOMPARE(projection.boundaryScope, KiriView::ActiveNavigationBoundaryScope::ImageDocument);
    QVERIFY(projection.activeNavigation.known);
    QVERIFY(projection.activeNavigation.canOpenPrevious);
    QVERIFY(projection.activeNavigation.canOpenNext);
    QCOMPARE(projection.activeNavigation.currentNumber, 2);
    QCOMPARE(projection.activeNavigation.count, 5);
    QCOMPARE(projection.windowTitleSubject, QStringLiteral("book.cbz – 2/5"));
}

void TestDocumentSessionPublicProjection::
    deletionMasksNavigationDispatchWithoutDroppingTitleCounter()
{
    const KiriView::DocumentSessionPublicProjection projection
        = KiriView::projectDocumentSessionPublicState(
            KiriView::DocumentSessionPublicProjectionInput {
                KiriView::DocumentSessionKind::Video,
                false,
                false,
                true,
                KiriView::MediaActiveNavigationInput {
                    KiriView::MediaNavigationBoundaryState { true, true, false, false, 2, 4 },
                    true,
                },
                {},
                {},
                {},
                QStringLiteral("clip.mp4"),
                QSize(1920, 1080),
            });

    QCOMPARE(projection.sourceKind, KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia);
    QVERIFY(projection.activeNavigation.known);
    QVERIFY(!projection.activeNavigation.editable);
    QVERIFY(!projection.activeNavigation.canOpenPrevious);
    QVERIFY(!projection.activeNavigation.canOpenNext);
    QCOMPARE(projection.activeNavigation.currentNumber, 2);
    QCOMPARE(projection.activeNavigation.count, 4);
    QCOMPARE(projection.windowTitleSubject, QStringLiteral("clip.mp4 – 1920×1080"));
}

QTEST_GUILESS_MAIN(TestDocumentSessionPublicProjection)

#include "test_documentsessionpublicprojection.moc"
