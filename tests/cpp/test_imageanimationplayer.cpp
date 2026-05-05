// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageanimationplayer.h"

#include <QObject>
#include <QString>
#include <QTest>
#include <Qt>
#include <vector>

namespace {
KiriView::AnimationFrame frame(int delay)
{
    QImage image(1, 1, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return KiriView::AnimationFrame { image, delay };
}
}

class TestImageAnimationPlayer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void replacingActiveDecodedPlaybackStopsPendingTimer();
};

void TestImageAnimationPlayer::replacingActiveDecodedPlaybackStopsPendingTimer()
{
    int readyFrameCount = 0;
    QString errorString;
    KiriView::ImageAnimationPlayer player(
        this, [&readyFrameCount](const QImage &) { ++readyFrameCount; },
        [&errorString](const QString &error) { errorString = error; });

    player.startDecoded({ frame(10), frame(10) }, -1);
    player.startDecoded({ frame(10) }, -1);

    QTest::qWait(50);

    QCOMPARE(readyFrameCount, 0);
    QVERIFY(errorString.isEmpty());
}

QTEST_GUILESS_MAIN(TestImageAnimationPlayer)

#include "test_imageanimationplayer.moc"
