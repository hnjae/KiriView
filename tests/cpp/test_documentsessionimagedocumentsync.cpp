// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionimagedocumentsync.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDocumentSessionImageDocumentSync : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ignoresRoutingAndInactiveDocumentKind();
    void mirrorsDocumentSourceAndDeletionProgress();
    void refreshesDirectMediaNavigationWhenCursorChanges();
    void cachesDisplayedPredecodeWhenNavigationIsKnown();
    void publishesImagePagesWhenPageNavigationChanges();
};

namespace {
kiriview::DocumentSessionImageDocumentSyncInput activeInput()
{
    kiriview::DocumentSessionImageDocumentSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.directMediaNavigationActive = true;
    input.image.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    return input;
}
}

void TestDocumentSessionImageDocumentSync::ignoresRoutingAndInactiveDocumentKind()
{
    kiriview::DocumentSessionImageDocumentSyncInput input = activeInput();
    input.routingSource = true;

    kiriview::DocumentSessionImageDocumentSyncPlan plan
        = kiriview::documentSessionImageDocumentSyncPlan(input);

    QVERIFY(!plan.active);
    QCOMPARE(plan.projectionOperation,
        kiriview::DocumentSessionImageDocumentSyncProjectionOperation::None);

    input = activeInput();
    input.documentKind = kiriview::DocumentSessionKind::Video;
    plan = kiriview::documentSessionImageDocumentSyncPlan(input);

    QVERIFY(!plan.active);
    QCOMPARE(plan.directMediaOperation,
        kiriview::DocumentSessionImageDocumentSyncDirectMediaOperation::None);
}

void TestDocumentSessionImageDocumentSync::mirrorsDocumentSourceAndDeletionProgress()
{
    kiriview::DocumentSessionImageDocumentSyncInput input = activeInput();
    input.directImageLoadMayUseImageDocumentSourceScope = false;
    input.directMediaNavigationActive = false;
    input.image.fileDeletionInProgress = true;

    const kiriview::DocumentSessionImageDocumentSyncPlan plan
        = kiriview::documentSessionImageDocumentSyncPlan(input);

    QVERIFY(plan.active);
    QVERIFY(plan.setSourceIdentity);
    QCOMPARE(plan.sourceIdentityUrl, input.image.sourceUrl);
    QVERIFY(plan.syncFileDeletionProgress);
    QVERIFY(plan.fileDeletionInProgress);
    QCOMPARE(plan.directMediaOperation,
        kiriview::DocumentSessionImageDocumentSyncDirectMediaOperation::
            RefreshDirectMediaNavigation);
    QCOMPARE(plan.projectionOperation,
        kiriview::DocumentSessionImageDocumentSyncProjectionOperation::RecomputePublicProjection);
}

void TestDocumentSessionImageDocumentSync::refreshesDirectMediaNavigationWhenCursorChanges()
{
    kiriview::DocumentSessionImageDocumentSyncInput input = activeInput();
    input.directImageLoadMayUseImageDocumentSourceScope = true;
    input.directMediaNavigationKnown = true;
    input.directMediaScopeChanged = true;
    input.image.fileDeletionInProgress = true;

    const kiriview::DocumentSessionImageDocumentSyncPlan plan
        = kiriview::documentSessionImageDocumentSyncPlan(input);

    QVERIFY(plan.active);
    QVERIFY(!plan.syncFileDeletionProgress);
    QCOMPARE(plan.directMediaOperation,
        kiriview::DocumentSessionImageDocumentSyncDirectMediaOperation::
            RefreshDirectMediaNavigation);
}

void TestDocumentSessionImageDocumentSync::cachesDisplayedPredecodeWhenNavigationIsKnown()
{
    kiriview::DocumentSessionImageDocumentSyncInput input = activeInput();
    input.directMediaNavigationKnown = true;

    const kiriview::DocumentSessionImageDocumentSyncPlan plan
        = kiriview::documentSessionImageDocumentSyncPlan(input);

    QCOMPARE(plan.directMediaOperation,
        kiriview::DocumentSessionImageDocumentSyncDirectMediaOperation::
            CacheDisplayedMediaPredecodeImages);
    QCOMPARE(plan.projectionOperation,
        kiriview::DocumentSessionImageDocumentSyncProjectionOperation::RecomputePublicProjection);
}

void TestDocumentSessionImageDocumentSync::publishesImagePagesWhenPageNavigationChanges()
{
    kiriview::DocumentSessionImageDocumentSyncInput input = activeInput();
    input.previousPageNavigation.known = false;
    input.image.pageNavigation.known = true;
    input.image.pageNavigation.currentNumber = 2;
    input.image.pageNavigation.count = 3;

    const kiriview::DocumentSessionImageDocumentSyncPlan plan
        = kiriview::documentSessionImageDocumentSyncPlan(input);

    QCOMPARE(plan.projectionOperation,
        kiriview::DocumentSessionImageDocumentSyncProjectionOperation::
            PublishImagePageActiveNavigation);
}

QTEST_GUILESS_MAIN(TestDocumentSessionImageDocumentSync)

#include "test_documentsessionimagedocumentsync.moc"
