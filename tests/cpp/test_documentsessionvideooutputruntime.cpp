// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionvideooutputruntime.h"

#include <QObject>
#include <QRectF>
#include <QTest>

class TestDocumentSessionVideoOutputRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void appliesAcceptedAttachAndDetachThroughPort();
    void rejectsStaleInvalidAndForeignClaimsWithoutTouchingPort();
    void clearForgetsActiveClaim();
};

namespace {
struct AttachmentProbe {
    kiriview::DocumentSessionVideoOutputAttachmentPort port()
    {
        return kiriview::DocumentSessionVideoOutputAttachmentPort {
            [this](QObject *videoOutput) {
                attachedVideoOutput = videoOutput;
                ++setVideoOutputCount;
            },
            [this](const QRectF &contentRect, const QRectF &sourceRect) {
                lastContentRect = contentRect;
                lastSourceRect = sourceRect;
                ++setGeometryCount;
            },
        };
    }

    QObject *attachedVideoOutput = nullptr;
    QRectF lastContentRect;
    QRectF lastSourceRect;
    int setVideoOutputCount = 0;
    int setGeometryCount = 0;
};
}

void TestDocumentSessionVideoOutputRuntime::appliesAcceptedAttachAndDetachThroughPort()
{
    kiriview::DocumentSessionVideoOutputRuntime runtime;
    QObject owner;
    QObject videoOutput;
    AttachmentProbe probe;
    const QRectF contentRect(1.0, 2.0, 320.0, 180.0);
    const QRectF sourceRect(3.0, 4.0, 640.0, 360.0);

    QVERIFY(runtime.reportSurfaceClaim(
        { runtime.nextSurfaceClaimToken(), &owner, &videoOutput, true, contentRect, sourceRect },
        probe.port()));
    QCOMPARE(probe.attachedVideoOutput, &videoOutput);
    QCOMPARE(probe.lastContentRect, contentRect);
    QCOMPARE(probe.lastSourceRect, sourceRect);
    QCOMPARE(probe.setVideoOutputCount, 1);
    QCOMPARE(probe.setGeometryCount, 1);

    QVERIFY(runtime.reportSurfaceClaim(
        { runtime.nextSurfaceClaimToken(), &owner, nullptr, false, {}, {} }, probe.port()));
    QCOMPARE(probe.attachedVideoOutput, nullptr);
    QCOMPARE(probe.setVideoOutputCount, 2);
    QCOMPARE(probe.setGeometryCount, 1);
}

void TestDocumentSessionVideoOutputRuntime::rejectsStaleInvalidAndForeignClaimsWithoutTouchingPort()
{
    kiriview::DocumentSessionVideoOutputRuntime runtime;
    QObject owner;
    QObject otherOwner;
    QObject videoOutput;
    AttachmentProbe probe;

    const QString staleToken = runtime.nextSurfaceClaimToken();
    const QString attachToken = runtime.nextSurfaceClaimToken();
    QVERIFY(runtime.reportSurfaceClaim(
        { attachToken, &owner, &videoOutput, true, {}, {} }, probe.port()));

    probe = AttachmentProbe {};
    QVERIFY(
        !runtime.reportSurfaceClaim({ staleToken, &owner, nullptr, false, {}, {} }, probe.port()));
    QVERIFY(!runtime.reportSurfaceClaim(
        { QStringLiteral("not-a-token"), &owner, &videoOutput, true, {}, {} }, probe.port()));
    QVERIFY(!runtime.reportSurfaceClaim(
        { runtime.nextSurfaceClaimToken(), nullptr, &videoOutput, true, {}, {} }, probe.port()));
    QVERIFY(!runtime.reportSurfaceClaim(
        { runtime.nextSurfaceClaimToken(), &otherOwner, nullptr, false, {}, {} }, probe.port()));
    QCOMPARE(probe.setVideoOutputCount, 0);
    QCOMPARE(probe.setGeometryCount, 0);
}

void TestDocumentSessionVideoOutputRuntime::clearForgetsActiveClaim()
{
    kiriview::DocumentSessionVideoOutputRuntime runtime;
    QObject owner;
    QObject videoOutput;
    AttachmentProbe probe;

    QVERIFY(runtime.reportSurfaceClaim(
        { runtime.nextSurfaceClaimToken(), &owner, &videoOutput, true, {}, {} }, probe.port()));

    runtime.clear();

    probe = AttachmentProbe {};
    QVERIFY(!runtime.reportSurfaceClaim(
        { runtime.nextSurfaceClaimToken(), &owner, nullptr, false, {}, {} }, probe.port()));
    QCOMPARE(probe.setVideoOutputCount, 0);
    QCOMPARE(probe.setGeometryCount, 0);
}

QTEST_GUILESS_MAIN(TestDocumentSessionVideoOutputRuntime)

#include "test_documentsessionvideooutputruntime.moc"
