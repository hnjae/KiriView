// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/windowtitleprojection.h"

#include <QObject>
#include <QSize>
#include <QTest>

class TestWindowTitleProjection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyBaseNameProducesEmptySubject();
    void directMediaUsesKnownIntrinsicSize();
    void directMediaWithUnknownSizeFallsBackToName();
    void documentPagesUseKnownNavigationCounter();
    void documentPagesWithUnknownNavigationFallsBackToName();
};

void TestWindowTitleProjection::emptyBaseNameProducesEmptySubject()
{
    QCOMPARE(KiriView::projectWindowTitleSubject(KiriView::WindowTitleSubjectInput {}), QString());
}

void TestWindowTitleProjection::directMediaUsesKnownIntrinsicSize()
{
    QCOMPARE(KiriView::projectWindowTitleSubject(KiriView::WindowTitleSubjectInput {
                 QStringLiteral("photo.png"),
                 KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
                 QSize(1920, 1080),
                 {},
             }),
        QStringLiteral("photo.png – 1920×1080"));
}

void TestWindowTitleProjection::directMediaWithUnknownSizeFallsBackToName()
{
    QCOMPARE(KiriView::projectWindowTitleSubject(KiriView::WindowTitleSubjectInput {
                 QStringLiteral("clip.mp4"),
                 KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
                 QSize(),
                 {},
             }),
        QStringLiteral("clip.mp4"));
}

void TestWindowTitleProjection::documentPagesUseKnownNavigationCounter()
{
    KiriView::ActiveNavigationSnapshot navigation;
    navigation.available = true;
    navigation.known = true;
    navigation.currentNumber = 12;
    navigation.count = 80;

    QCOMPARE(KiriView::projectWindowTitleSubject(KiriView::WindowTitleSubjectInput {
                 QStringLiteral("book.cbz"),
                 KiriView::ActiveNavigationSourceKind::ImageDocumentPages,
                 {},
                 navigation,
             }),
        QStringLiteral("book.cbz – 12/80"));
}

void TestWindowTitleProjection::documentPagesWithUnknownNavigationFallsBackToName()
{
    KiriView::ActiveNavigationSnapshot navigation;
    navigation.available = true;

    QCOMPARE(KiriView::projectWindowTitleSubject(KiriView::WindowTitleSubjectInput {
                 QStringLiteral("book.cbz"),
                 KiriView::ActiveNavigationSourceKind::ImageDocumentPages,
                 {},
                 navigation,
             }),
        QStringLiteral("book.cbz"));
}

QTEST_GUILESS_MAIN(TestWindowTitleProjection)

#include "test_windowtitleprojection.moc"
