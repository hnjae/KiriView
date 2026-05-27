// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "localization/activenavigationboundarytext.h"

#include <QObject>
#include <QTest>

class TestActiveNavigationBoundaryText : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void feedbackTextFollowsBoundaryScopeAndOutcome();
    void nonBoundaryOutcomesHaveNoFeedbackText();
};

void TestActiveNavigationBoundaryText::feedbackTextFollowsBoundaryScopeAndOutcome()
{
    QCOMPARE(KiriView::activeNavigationBoundaryFeedbackText(
                 KiriView::ActiveNavigationBoundaryScope::Media,
                 KiriView::ActiveNavigationDispatchOutcome::FirstBoundary),
        QStringLiteral("First media item"));
    QCOMPARE(KiriView::activeNavigationBoundaryFeedbackText(
                 KiriView::ActiveNavigationBoundaryScope::Media,
                 KiriView::ActiveNavigationDispatchOutcome::LastBoundary),
        QStringLiteral("Last media item"));
    QCOMPARE(KiriView::activeNavigationBoundaryFeedbackText(
                 KiriView::ActiveNavigationBoundaryScope::ImageDocument,
                 KiriView::ActiveNavigationDispatchOutcome::FirstBoundary),
        QStringLiteral("First image"));
    QCOMPARE(KiriView::activeNavigationBoundaryFeedbackText(
                 KiriView::ActiveNavigationBoundaryScope::ImageDocument,
                 KiriView::ActiveNavigationDispatchOutcome::LastBoundary),
        QStringLiteral("Last image"));
}

void TestActiveNavigationBoundaryText::nonBoundaryOutcomesHaveNoFeedbackText()
{
    QCOMPARE(KiriView::activeNavigationBoundaryFeedbackText(
                 KiriView::ActiveNavigationBoundaryScope::Media,
                 KiriView::ActiveNavigationDispatchOutcome::NoOp),
        QString());
    QCOMPARE(KiriView::activeNavigationBoundaryFeedbackText(
                 KiriView::ActiveNavigationBoundaryScope::ImageDocument,
                 KiriView::ActiveNavigationDispatchOutcome::Dispatch),
        QString());
    QCOMPARE(KiriView::activeNavigationBoundaryFeedbackText(
                 KiriView::ActiveNavigationBoundaryScope::None,
                 KiriView::ActiveNavigationDispatchOutcome::FirstBoundary),
        QString());
}

QTEST_GUILESS_MAIN(TestActiveNavigationBoundaryText)

#include "test_activenavigationboundarytext.moc"
