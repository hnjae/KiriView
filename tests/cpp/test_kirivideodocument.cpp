// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kirivideodocument.h"

#include "facade/kiridocumentsession.h"
#include <QMetaProperty>
#include <QObject>
#include <QRectF>
#include <QSignalSpy>
#include <QTest>

class TestKiriVideoDocument : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialStateIsNull();
    void sourceUrlAndVideoOutputPropertiesAreReadOnlyObservations();
    void mutedPropertyNotifiesAndToggles();
    void sourceUrlAndTitleNotifyOnSetAndClear();
    void videoOutputCanDetachAndToleratesDestroyedOutput();
};

void TestKiriVideoDocument::initialStateIsNull()
{
    KiriVideoDocument document;

    QCOMPARE(document.sourceUrl(), QUrl());
    QCOMPARE(document.status(), KiriVideoDocument::Status::Null);
    QCOMPARE(document.errorString(), QString());
    QCOMPARE(document.windowTitleFileName(), QString());
    QCOMPARE(document.duration(), 0);
    QCOMPARE(document.position(), 0);
    QVERIFY(!document.playing());
    QVERIFY(!document.seekable());
    QVERIFY(!document.hasVideo());
    QVERIFY(!document.hasAudio());
    QCOMPARE(document.videoSize(), QSize());
    QVERIFY(!document.zoomPercentKnown());
    QCOMPARE(document.zoomPercent(), 0);
    QVERIFY(!document.muted());
    QCOMPARE(document.videoOutput(), nullptr);
}

void TestKiriVideoDocument::sourceUrlAndVideoOutputPropertiesAreReadOnlyObservations()
{
    const QMetaObject &metaObject = KiriVideoDocument::staticMetaObject;
    const int sourceUrlIndex = metaObject.indexOfProperty("sourceUrl");
    QVERIFY(sourceUrlIndex >= 0);

    const QMetaProperty sourceUrlProperty = metaObject.property(sourceUrlIndex);
    QVERIFY(sourceUrlProperty.hasNotifySignal());
    QVERIFY(!sourceUrlProperty.isWritable());

    const int videoOutputIndex = metaObject.indexOfProperty("videoOutput");
    QVERIFY(videoOutputIndex >= 0);

    const QMetaProperty videoOutputProperty = metaObject.property(videoOutputIndex);
    QVERIFY(videoOutputProperty.hasNotifySignal());
    QVERIFY(!videoOutputProperty.isWritable());
}

void TestKiriVideoDocument::mutedPropertyNotifiesAndToggles()
{
    KiriVideoDocument document;
    QSignalSpy mutedSpy(&document, &KiriVideoDocument::mutedChanged);

    document.setMuted(true);
    QVERIFY(document.muted());
    QCOMPARE(mutedSpy.count(), 1);

    document.setMuted(true);
    QCOMPARE(mutedSpy.count(), 1);

    document.toggleMuted();
    QVERIFY(!document.muted());
    QCOMPARE(mutedSpy.count(), 2);
}

void TestKiriVideoDocument::sourceUrlAndTitleNotifyOnSetAndClear()
{
    KiriDocumentSession session;
    KiriVideoDocument &document = *session.videoDocument();
    QSignalSpy sourceUrlSpy(&document, &KiriVideoDocument::sourceUrlChanged);
    QSignalSpy titleSpy(&document, &KiriVideoDocument::windowTitleFileNameChanged);
    QSignalSpy statusSpy(&document, &KiriVideoDocument::statusChanged);
    const QUrl sourceUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/clip.mp4"));

    session.setSourceUrl(sourceUrl);
    QCOMPARE(document.sourceUrl(), sourceUrl);
    QCOMPARE(document.windowTitleFileName(), QStringLiteral("clip.mp4"));
    QCOMPARE(document.status(), KiriVideoDocument::Status::Loading);
    QCOMPARE(sourceUrlSpy.count(), 1);
    QCOMPARE(titleSpy.count(), 1);
    QVERIFY(statusSpy.count() >= 1);

    session.setSourceUrl(QUrl());
    QCOMPARE(document.sourceUrl(), QUrl());
    QCOMPARE(document.windowTitleFileName(), QString());
    QCOMPARE(document.status(), KiriVideoDocument::Status::Null);
    QCOMPARE(sourceUrlSpy.count(), 2);
    QCOMPARE(titleSpy.count(), 2);
}

void TestKiriVideoDocument::videoOutputCanDetachAndToleratesDestroyedOutput()
{
    KiriDocumentSession session;
    KiriVideoDocument &document = *session.videoDocument();
    QSignalSpy videoOutputSpy(&document, &KiriVideoDocument::videoOutputChanged);
    auto *output = new QObject();
    QObject surfaceOwner;
    QObject staleSurfaceOwner;
    const QUrl sourceUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/clip.mp4"));
    const QUrl replacementSourceUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/replacement.mp4"));

    session.setSourceUrl(sourceUrl);
    QCOMPARE(session.documentKind(), KiriDocumentSession::DocumentKind::Video);
    const quint64 staleProjectionRevision = session.publicProjectionRevision();
    const QString staleProjectionClaimToken = session.nextVideoOutputSurfaceClaimToken();

    session.setSourceUrl(replacementSourceUrl);
    QVERIFY(session.publicProjectionRevision() > staleProjectionRevision);
    QVERIFY(!session.reportVideoOutputSurfaceClaim(staleProjectionClaimToken,
        staleProjectionRevision, &staleSurfaceOwner, output, true, QRectF(), QRectF()));
    QCOMPARE(document.videoOutput(), nullptr);
    QCOMPARE(videoOutputSpy.count(), 0);

    QVERIFY(!session.reportVideoOutputSurfaceClaim(session.nextVideoOutputSurfaceClaimToken(),
        session.publicProjectionRevision(), nullptr, output, true, QRectF(), QRectF()));
    QCOMPARE(document.videoOutput(), nullptr);
    QCOMPARE(videoOutputSpy.count(), 0);

    QVERIFY(!session.reportVideoOutputSurfaceClaim(QStringLiteral("not-a-claim-token"),
        session.publicProjectionRevision(), &surfaceOwner, output, true, QRectF(), QRectF()));
    QCOMPARE(document.videoOutput(), nullptr);
    QCOMPARE(videoOutputSpy.count(), 0);

    const QString staleSameOwnerDetachToken = session.nextVideoOutputSurfaceClaimToken();
    const QString activeAttachToken = session.nextVideoOutputSurfaceClaimToken();
    QVERIFY(session.reportVideoOutputSurfaceClaim(activeAttachToken,
        session.publicProjectionRevision(), &surfaceOwner, output, true, QRectF(), QRectF()));
    session.reportVideoOutputSurfaceClaim(session.nextVideoOutputSurfaceClaimToken(),
        session.publicProjectionRevision(), &staleSurfaceOwner, nullptr, false, QRectF(), QRectF());
    QCOMPARE(document.videoOutput(), output);
    QCOMPARE(videoOutputSpy.count(), 1);

    QVERIFY(!session.reportVideoOutputSurfaceClaim(staleSameOwnerDetachToken,
        session.publicProjectionRevision(), &surfaceOwner, nullptr, false, QRectF(), QRectF()));
    QCOMPARE(document.videoOutput(), output);
    QCOMPARE(videoOutputSpy.count(), 1);

    QVERIFY(session.reportVideoOutputSurfaceClaim(session.nextVideoOutputSurfaceClaimToken(),
        session.publicProjectionRevision(), &surfaceOwner, nullptr, false, QRectF(), QRectF()));
    QCOMPARE(document.videoOutput(), nullptr);
    QCOMPARE(videoOutputSpy.count(), 2);

    output = new QObject();
    QVERIFY(session.reportVideoOutputSurfaceClaim(session.nextVideoOutputSurfaceClaimToken(),
        session.publicProjectionRevision(), &surfaceOwner, output, true, QRectF(), QRectF()));
    QCOMPARE(document.videoOutput(), output);
    delete output;
    QCOMPARE(document.videoOutput(), nullptr);
    QCOMPARE(videoOutputSpy.count(), 4);
}

QTEST_GUILESS_MAIN(TestKiriVideoDocument)

#include "test_kirivideodocument.moc"
