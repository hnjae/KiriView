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
    QCOMPARE(kiriview::activeNavigationBoundaryFeedbackText(
                 kiriview::ActiveNavigationBoundaryScope::DirectMedia,
                 kiriview::ActiveNavigationDispatchOutcome::FirstBoundary),
        QStringLiteral("First media item"));
    QCOMPARE(kiriview::activeNavigationBoundaryFeedbackText(
                 kiriview::ActiveNavigationBoundaryScope::DirectMedia,
                 kiriview::ActiveNavigationDispatchOutcome::LastBoundary),
        QStringLiteral("Last media item"));
    QCOMPARE(kiriview::activeNavigationBoundaryFeedbackText(
                 kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage,
                 kiriview::ActiveNavigationDispatchOutcome::FirstBoundary),
        QStringLiteral("First image"));
    QCOMPARE(kiriview::activeNavigationBoundaryFeedbackText(
                 kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage,
                 kiriview::ActiveNavigationDispatchOutcome::LastBoundary),
        QStringLiteral("Last image"));
}

void TestActiveNavigationBoundaryText::nonBoundaryOutcomesHaveNoFeedbackText()
{
    QCOMPARE(kiriview::activeNavigationBoundaryFeedbackText(
                 kiriview::ActiveNavigationBoundaryScope::DirectMedia,
                 kiriview::ActiveNavigationDispatchOutcome::NoOp),
        QString());
    QCOMPARE(kiriview::activeNavigationBoundaryFeedbackText(
                 kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage,
                 kiriview::ActiveNavigationDispatchOutcome::Dispatch),
        QString());
    QCOMPARE(kiriview::activeNavigationBoundaryFeedbackText(
                 kiriview::ActiveNavigationBoundaryScope::None,
                 kiriview::ActiveNavigationDispatchOutcome::FirstBoundary),
        QString());
}

QTEST_GUILESS_MAIN(TestActiveNavigationBoundaryText)

#include "test_activenavigationboundarytext.moc"
