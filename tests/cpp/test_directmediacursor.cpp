// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/directmediacursor.h"

#include <QObject>
#include <QTest>
#include <QUrl>

class TestDirectMediaCursor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void generationChangesOnlyWithEffectiveIdentity();
    void generationUsesNormalizedEffectiveIdentity();
};

void TestDirectMediaCursor::generationChangesOnlyWithEffectiveIdentity()
{
    KiriView::DirectMediaCursor cursor;
    const QUrl firstImage = QUrl::fromLocalFile(QStringLiteral("/media/01.png"));
    const QUrl secondImage = QUrl::fromLocalFile(QStringLiteral("/media/02.png"));
    const QUrl video = QUrl::fromLocalFile(QStringLiteral("/media/03.mp4"));

    const quint64 initialGeneration = cursor.generation;

    KiriView::requestDirectImageCursor(cursor, firstImage);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), firstImage);
    QCOMPARE(cursor.pendingUrl, firstImage);
    QCOMPARE(cursor.stableUrl, QUrl());
    const quint64 requestedGeneration = cursor.generation;
    QVERIFY(requestedGeneration > initialGeneration);

    KiriView::requestDirectImageCursor(cursor, firstImage);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), firstImage);
    QCOMPARE(cursor.generation, requestedGeneration);

    KiriView::confirmDirectImageCursor(cursor, firstImage);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), firstImage);
    QCOMPARE(cursor.pendingUrl, QUrl());
    QCOMPARE(cursor.stableUrl, firstImage);
    const quint64 confirmedGeneration = cursor.generation;
    QCOMPARE(confirmedGeneration, requestedGeneration);

    KiriView::confirmDirectImageCursor(cursor, firstImage);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), firstImage);
    QCOMPARE(cursor.generation, confirmedGeneration);

    KiriView::requestDirectImageCursor(cursor, secondImage);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), secondImage);
    QCOMPARE(cursor.stableUrl, firstImage);
    QCOMPARE(cursor.pendingUrl, secondImage);
    const quint64 replacementGeneration = cursor.generation;
    QVERIFY(replacementGeneration > confirmedGeneration);

    KiriView::requestDirectImageCursor(cursor, secondImage);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), secondImage);
    QCOMPARE(cursor.generation, replacementGeneration);

    KiriView::restoreDirectImageCursorAfterFailure(cursor);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), firstImage);
    QCOMPARE(cursor.pendingUrl, QUrl());
    QCOMPARE(cursor.stableUrl, firstImage);
    const quint64 restoredGeneration = cursor.generation;
    QVERIFY(restoredGeneration > replacementGeneration);

    KiriView::restoreDirectImageCursorAfterFailure(cursor);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), firstImage);
    QCOMPARE(cursor.generation, restoredGeneration);

    KiriView::setDirectVideoCursor(cursor, video);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), video);
    QCOMPARE(cursor.stableUrl, video);
    const quint64 videoGeneration = cursor.generation;
    QVERIFY(videoGeneration > restoredGeneration);

    KiriView::setDirectVideoCursor(cursor, video);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), video);
    QCOMPARE(cursor.generation, videoGeneration);

    KiriView::clearDirectMediaCursor(cursor);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), QUrl());
    QCOMPARE(cursor.stableUrl, QUrl());
    const quint64 clearedGeneration = cursor.generation;
    QVERIFY(clearedGeneration > videoGeneration);

    KiriView::clearDirectMediaCursor(cursor);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), QUrl());
    QCOMPARE(cursor.generation, clearedGeneration);
}

void TestDirectMediaCursor::generationUsesNormalizedEffectiveIdentity()
{
    KiriView::DirectMediaCursor cursor;
    const QUrl requestedImage(QStringLiteral("file:///media/chapter/../01.png"));
    const QUrl displayedImage(QStringLiteral("file:///media/01.png"));
    const QUrl replacementImage(QStringLiteral("file:///media/02.png"));

    KiriView::requestDirectImageCursor(cursor, requestedImage);
    const quint64 requestedGeneration = cursor.generation;

    KiriView::confirmDirectImageCursor(cursor, displayedImage);
    QCOMPARE(KiriView::effectiveDirectMediaCursorUrl(cursor), displayedImage);
    QCOMPARE(cursor.generation, requestedGeneration);

    KiriView::requestDirectImageCursor(cursor, replacementImage);
    QVERIFY(cursor.generation > requestedGeneration);
}

QTEST_GUILESS_MAIN(TestDirectMediaCursor)

#include "test_directmediacursor.moc"
