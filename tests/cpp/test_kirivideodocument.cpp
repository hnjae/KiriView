// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kirivideodocument.h"

#include <QObject>
#include <QSignalSpy>
#include <QTest>

class TestKiriVideoDocument : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialStateIsNull();
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
    KiriVideoDocument document;
    QSignalSpy sourceUrlSpy(&document, &KiriVideoDocument::sourceUrlChanged);
    QSignalSpy titleSpy(&document, &KiriVideoDocument::windowTitleFileNameChanged);
    QSignalSpy statusSpy(&document, &KiriVideoDocument::statusChanged);
    const QUrl sourceUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/clip.mp4"));

    document.setSourceUrl(sourceUrl);
    QCOMPARE(document.sourceUrl(), sourceUrl);
    QCOMPARE(document.windowTitleFileName(), QStringLiteral("clip.mp4"));
    QCOMPARE(document.status(), KiriVideoDocument::Status::Loading);
    QCOMPARE(sourceUrlSpy.count(), 1);
    QCOMPARE(titleSpy.count(), 1);
    QVERIFY(statusSpy.count() >= 1);

    document.setSourceUrl(QUrl());
    QCOMPARE(document.sourceUrl(), QUrl());
    QCOMPARE(document.windowTitleFileName(), QString());
    QCOMPARE(document.status(), KiriVideoDocument::Status::Null);
    QCOMPARE(sourceUrlSpy.count(), 2);
    QCOMPARE(titleSpy.count(), 2);
}

void TestKiriVideoDocument::videoOutputCanDetachAndToleratesDestroyedOutput()
{
    KiriVideoDocument document;
    QSignalSpy videoOutputSpy(&document, &KiriVideoDocument::videoOutputChanged);
    auto *output = new QObject();

    document.setVideoOutput(output);
    QCOMPARE(document.videoOutput(), output);
    QCOMPARE(videoOutputSpy.count(), 1);

    document.setVideoOutput(nullptr);
    QCOMPARE(document.videoOutput(), nullptr);
    QCOMPARE(videoOutputSpy.count(), 2);

    output = new QObject();
    document.setVideoOutput(output);
    QCOMPARE(document.videoOutput(), output);
    delete output;
    QCOMPARE(document.videoOutput(), nullptr);
    QCOMPARE(videoOutputSpy.count(), 4);
}

QTEST_GUILESS_MAIN(TestKiriVideoDocument)

#include "test_kirivideodocument.moc"
