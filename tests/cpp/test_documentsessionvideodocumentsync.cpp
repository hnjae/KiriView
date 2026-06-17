// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionvideodocumentsync.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDocumentSessionVideoDocumentSync : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ignoresInactiveDocumentKind();
    void clearsSessionWhenVideoSourceBecomesEmpty();
    void commitsDirectVideoCursorWhenVideoSourceIsPresent();
};

void TestDocumentSessionVideoDocumentSync::ignoresInactiveDocumentKind()
{
    kiriview::DocumentSessionVideoDocumentSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Image;
    input.video.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mkv"));

    const kiriview::DocumentSessionVideoDocumentSyncPlan plan
        = kiriview::documentSessionVideoDocumentSyncPlan(input);

    QCOMPARE(plan.operation, kiriview::DocumentSessionVideoDocumentSyncOperation::None);
    QCOMPARE(plan.url, QUrl());
}

void TestDocumentSessionVideoDocumentSync::clearsSessionWhenVideoSourceBecomesEmpty()
{
    kiriview::DocumentSessionVideoDocumentSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Video;

    const kiriview::DocumentSessionVideoDocumentSyncPlan plan
        = kiriview::documentSessionVideoDocumentSyncPlan(input);

    QCOMPARE(plan.operation,
        kiriview::DocumentSessionVideoDocumentSyncOperation::ClearSessionDirectMedia);
    QCOMPARE(plan.url, QUrl());
}

void TestDocumentSessionVideoDocumentSync::commitsDirectVideoCursorWhenVideoSourceIsPresent()
{
    kiriview::DocumentSessionVideoDocumentSyncInput input;
    input.documentKind = kiriview::DocumentSessionKind::Video;
    input.video.sourceUrl = QUrl::fromLocalFile(QStringLiteral("/media/clip.mkv"));

    const kiriview::DocumentSessionVideoDocumentSyncPlan plan
        = kiriview::documentSessionVideoDocumentSyncPlan(input);

    QCOMPARE(plan.operation,
        kiriview::DocumentSessionVideoDocumentSyncOperation::CommitDirectVideoCursor);
    QCOMPARE(plan.url, input.video.sourceUrl);
}

QTEST_GUILESS_MAIN(TestDocumentSessionVideoDocumentSync)

#include "test_documentsessionvideodocumentsync.moc"
