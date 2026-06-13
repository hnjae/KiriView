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
    QCOMPARE(kiriview::projectWindowTitleSubject(kiriview::WindowTitleSubjectInput {}), QString());
}

void TestWindowTitleProjection::directMediaUsesKnownIntrinsicSize()
{
    QCOMPARE(kiriview::projectWindowTitleSubject(kiriview::WindowTitleSubjectInput {
                 QStringLiteral("photo.png"),
                 kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
                 QSize(1920, 1080),
                 {},
             }),
        QStringLiteral("photo.png – 1920×1080"));
}

void TestWindowTitleProjection::directMediaWithUnknownSizeFallsBackToName()
{
    QCOMPARE(kiriview::projectWindowTitleSubject(kiriview::WindowTitleSubjectInput {
                 QStringLiteral("clip.mp4"),
                 kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
                 QSize(),
                 {},
             }),
        QStringLiteral("clip.mp4"));
}

void TestWindowTitleProjection::documentPagesUseKnownNavigationCounter()
{
    kiriview::ActiveNavigationSnapshot navigation;
    navigation.available = true;
    navigation.known = true;
    navigation.currentNumber = 12;
    navigation.count = 80;

    QCOMPARE(kiriview::projectWindowTitleSubject(kiriview::WindowTitleSubjectInput {
                 QStringLiteral("book.cbz"),
                 kiriview::ActiveNavigationSourceKind::ImageDocumentPages,
                 {},
                 navigation,
             }),
        QStringLiteral("book.cbz – 12/80"));
}

void TestWindowTitleProjection::documentPagesWithUnknownNavigationFallsBackToName()
{
    kiriview::ActiveNavigationSnapshot navigation;
    navigation.available = true;

    QCOMPARE(kiriview::projectWindowTitleSubject(kiriview::WindowTitleSubjectInput {
                 QStringLiteral("book.cbz"),
                 kiriview::ActiveNavigationSourceKind::ImageDocumentPages,
                 {},
                 navigation,
             }),
        QStringLiteral("book.cbz"));
}

QTEST_GUILESS_MAIN(TestWindowTitleProjection)

#include "test_windowtitleprojection.moc"
