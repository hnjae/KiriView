// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionvideooutputruntime.h"

#include <QObject>
#include <QTest>

class TestDocumentSessionVideoOutputRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void acceptsNewerAttachAndRejectsStaleSameOwnerDetach();
    void rejectsInvalidClaimsAndForeignDetach();
    void clearForgetsActiveClaim();
};

void TestDocumentSessionVideoOutputRuntime::acceptsNewerAttachAndRejectsStaleSameOwnerDetach()
{
    kiriview::DocumentSessionVideoOutputRuntime runtime;
    QObject owner;

    const QString staleToken = runtime.nextSurfaceClaimToken();
    const QString attachToken = runtime.nextSurfaceClaimToken();
    QCOMPARE(runtime.reportSurfaceClaim({ attachToken, &owner, true }),
        kiriview::DocumentSessionVideoOutputClaimAction::Attach);

    QCOMPARE(runtime.reportSurfaceClaim({ staleToken, &owner, false }),
        kiriview::DocumentSessionVideoOutputClaimAction::Reject);

    QCOMPARE(runtime.reportSurfaceClaim({ runtime.nextSurfaceClaimToken(), &owner, false }),
        kiriview::DocumentSessionVideoOutputClaimAction::Detach);
}

void TestDocumentSessionVideoOutputRuntime::rejectsInvalidClaimsAndForeignDetach()
{
    kiriview::DocumentSessionVideoOutputRuntime runtime;
    QObject owner;
    QObject otherOwner;

    QCOMPARE(runtime.reportSurfaceClaim({ QStringLiteral("not-a-token"), &owner, true }),
        kiriview::DocumentSessionVideoOutputClaimAction::Reject);
    QCOMPARE(runtime.reportSurfaceClaim({ runtime.nextSurfaceClaimToken(), nullptr, true }),
        kiriview::DocumentSessionVideoOutputClaimAction::Reject);

    QCOMPARE(runtime.reportSurfaceClaim({ runtime.nextSurfaceClaimToken(), &owner, true }),
        kiriview::DocumentSessionVideoOutputClaimAction::Attach);
    QCOMPARE(runtime.reportSurfaceClaim({ runtime.nextSurfaceClaimToken(), &otherOwner, false }),
        kiriview::DocumentSessionVideoOutputClaimAction::Reject);
}

void TestDocumentSessionVideoOutputRuntime::clearForgetsActiveClaim()
{
    kiriview::DocumentSessionVideoOutputRuntime runtime;
    QObject owner;

    QCOMPARE(runtime.reportSurfaceClaim({ runtime.nextSurfaceClaimToken(), &owner, true }),
        kiriview::DocumentSessionVideoOutputClaimAction::Attach);

    runtime.clear();

    QCOMPARE(runtime.reportSurfaceClaim({ runtime.nextSurfaceClaimToken(), &owner, false }),
        kiriview::DocumentSessionVideoOutputClaimAction::Reject);
}

QTEST_GUILESS_MAIN(TestDocumentSessionVideoOutputRuntime)

#include "test_documentsessionvideooutputruntime.moc"
