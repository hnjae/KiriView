// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectimagecursorsync.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDocumentSessionDirectImageCursorSync : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void confirmsPendingCursorWhenDisplayedImageMatches();
    void confirmsPendingCursorWhenMatchingPendingLoadFails();
    void restoresStableCursorWhenPendingLoadFailsForAnotherSource();
    void restoresStableCursorWhenLoadedSourceDivergesFromPending();
    void confirmsDisplayedImageWithoutPendingCursor();
    void ignoresInactiveDocumentKind();
};

void TestDocumentSessionDirectImageCursorSync::confirmsPendingCursorWhenDisplayedImageMatches()
{
    kiriview::DocumentSessionDirectImageCursorSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.cursor.pendingUrl = QUrl(QStringLiteral("file:///media/chapter/../01.png"));
    input.image.ordinaryDirectMediaScopeActive = true;
    input.image.displayedUrl = QUrl(QStringLiteral("file:///media/01.png"));

    const kiriview::DocumentSessionDirectImageCursorSyncPlan plan
        = kiriview::documentSessionDirectImageCursorSyncPlan(input);

    QCOMPARE(plan.operation,
        kiriview::DocumentSessionDirectImageCursorSyncOperation::ConfirmDirectImageCursor);
    QCOMPARE(plan.url, input.image.displayedUrl);
}

void TestDocumentSessionDirectImageCursorSync::confirmsPendingCursorWhenMatchingPendingLoadFails()
{
    kiriview::DocumentSessionDirectImageCursorSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.cursor.pendingUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    input.image.sourceUrl = input.cursor.pendingUrl;
    input.image.error = true;

    const kiriview::DocumentSessionDirectImageCursorSyncPlan plan
        = kiriview::documentSessionDirectImageCursorSyncPlan(input);

    QCOMPARE(plan.operation,
        kiriview::DocumentSessionDirectImageCursorSyncOperation::ConfirmDirectImageCursor);
    QCOMPARE(plan.url, input.cursor.pendingUrl);
}

void TestDocumentSessionDirectImageCursorSync::
    restoresStableCursorWhenPendingLoadFailsForAnotherSource()
{
    kiriview::DocumentSessionDirectImageCursorSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.cursor.stableUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    input.cursor.pendingUrl = QUrl::fromLocalFile(QStringLiteral("/media/02.png"));
    input.image.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/other.png"));
    input.image.error = true;

    const kiriview::DocumentSessionDirectImageCursorSyncPlan plan
        = kiriview::documentSessionDirectImageCursorSyncPlan(input);

    QCOMPARE(plan.operation,
        kiriview::DocumentSessionDirectImageCursorSyncOperation::
            RestoreDirectImageCursorAfterFailure);
    QCOMPARE(plan.url, QUrl());
}

void TestDocumentSessionDirectImageCursorSync::
    restoresStableCursorWhenLoadedSourceDivergesFromPending()
{
    kiriview::DocumentSessionDirectImageCursorSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.cursor.stableUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    input.cursor.pendingUrl = QUrl::fromLocalFile(QStringLiteral("/media/02.png"));
    input.image.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/03.png"));

    const kiriview::DocumentSessionDirectImageCursorSyncPlan plan
        = kiriview::documentSessionDirectImageCursorSyncPlan(input);

    QCOMPARE(plan.operation,
        kiriview::DocumentSessionDirectImageCursorSyncOperation::
            RestoreDirectImageCursorAfterFailure);
    QCOMPARE(plan.url, QUrl());
}

void TestDocumentSessionDirectImageCursorSync::confirmsDisplayedImageWithoutPendingCursor()
{
    kiriview::DocumentSessionDirectImageCursorSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.image.ordinaryDirectMediaScopeActive = true;
    input.image.displayedUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));

    const kiriview::DocumentSessionDirectImageCursorSyncPlan plan
        = kiriview::documentSessionDirectImageCursorSyncPlan(input);

    QCOMPARE(plan.operation,
        kiriview::DocumentSessionDirectImageCursorSyncOperation::ConfirmDirectImageCursor);
    QCOMPARE(plan.url, input.image.displayedUrl);
}

void TestDocumentSessionDirectImageCursorSync::ignoresInactiveDocumentKind()
{
    kiriview::DocumentSessionDirectImageCursorSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Video;
    input.cursor.pendingUrl = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    input.image.ordinaryDirectMediaScopeActive = true;
    input.image.displayedUrl = input.cursor.pendingUrl;

    const kiriview::DocumentSessionDirectImageCursorSyncPlan plan
        = kiriview::documentSessionDirectImageCursorSyncPlan(input);

    QCOMPARE(plan.operation, kiriview::DocumentSessionDirectImageCursorSyncOperation::None);
    QCOMPARE(plan.url, QUrl());
}

QTEST_GUILESS_MAIN(TestDocumentSessionDirectImageCursorSync)

#include "test_documentsessiondirectimagecursorsync.moc"
