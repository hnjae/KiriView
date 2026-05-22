// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationplayer.h"

#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
QImage frameImage(const QSize &size)
{
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

struct FakePlaybackSourceStats {
    int openCount = 0;
};

class FakePlaybackSource final : public KiriView::ImageAnimationPlaybackSource
{
public:
    FakePlaybackSource(QSize firstFrameSize, std::vector<QSize> frameSizes, int loopCount,
        bool restartable, std::shared_ptr<FakePlaybackSourceStats> stats)
        : m_firstFrameSize(firstFrameSize)
        , m_frameSizes(std::move(frameSizes))
        , m_loopCount(loopCount)
        , m_restartable(restartable)
        , m_stats(std::move(stats))
    {
    }

    std::optional<KiriView::ImageAnimationPlaybackOpenResult> open(QString *errorString) override
    {
        if (errorString != nullptr) {
            errorString->clear();
        }
        m_nextFrameIndex = 0;
        ++m_stats->openCount;
        return KiriView::ImageAnimationPlaybackOpenResult {
            frameImage(m_firstFrameSize),
            0,
            m_loopCount,
            hasMoreFrames(),
        };
    }

    std::optional<KiriView::AnimationFrame> readNextFrame(QString *errorString) override
    {
        if (errorString != nullptr) {
            errorString->clear();
        }
        if (!hasMoreFrames()) {
            return std::nullopt;
        }

        return KiriView::AnimationFrame {
            frameImage(m_frameSizes.at(m_nextFrameIndex++)),
            0,
        };
    }

    bool hasMoreFrames() const override { return m_nextFrameIndex < m_frameSizes.size(); }

    bool restartable() const override { return m_restartable; }

private:
    QSize m_firstFrameSize;
    std::vector<QSize> m_frameSizes;
    int m_loopCount = 0;
    bool m_restartable = false;
    std::shared_ptr<FakePlaybackSourceStats> m_stats;
    std::size_t m_nextFrameIndex = 0;
};
}

class TestImageAnimationPlayer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startConsumesFirstFrameAndEmitsSubsequentFrames();
    void restartableSourcesEmitFirstFrameOnLoopRestart();
    void nonRestartableSourcesDoNotReopenAtSequenceEnd();
};

void TestImageAnimationPlayer::startConsumesFirstFrameAndEmitsSubsequentFrames()
{
    std::vector<QSize> emittedFrameSizes;
    QString errorString;
    KiriView::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage &image) { emittedFrameSizes.push_back(image.size()); },
        [&errorString](const QString &error) { errorString = error; });

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        QSize(1, 1), std::vector<QSize> { QSize(2, 1) }, 0, true, stats));

    QCOMPARE(stats->openCount, 1);
    QVERIFY(emittedFrameSizes.empty());
    QTRY_COMPARE_WITH_TIMEOUT(emittedFrameSizes.size(), std::size_t(1), 100);
    QCOMPARE(emittedFrameSizes.at(0), QSize(2, 1));
    QVERIFY(errorString.isEmpty());
}

void TestImageAnimationPlayer::restartableSourcesEmitFirstFrameOnLoopRestart()
{
    std::vector<QSize> emittedFrameSizes;
    KiriView::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage &image) { emittedFrameSizes.push_back(image.size()); },
        [](const QString &) {});

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        QSize(1, 1), std::vector<QSize> { QSize(2, 1) }, 1, true, stats));

    QTRY_COMPARE_WITH_TIMEOUT(emittedFrameSizes.size(), std::size_t(3), 200);
    QCOMPARE(stats->openCount, 2);
    QCOMPARE(emittedFrameSizes.at(0), QSize(2, 1));
    QCOMPARE(emittedFrameSizes.at(1), QSize(1, 1));
    QCOMPARE(emittedFrameSizes.at(2), QSize(2, 1));
}

void TestImageAnimationPlayer::nonRestartableSourcesDoNotReopenAtSequenceEnd()
{
    std::vector<QSize> emittedFrameSizes;
    KiriView::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage &image) { emittedFrameSizes.push_back(image.size()); },
        [](const QString &) {});

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        QSize(1, 1), std::vector<QSize> { QSize(2, 1) }, -1, false, stats));

    QTRY_COMPARE_WITH_TIMEOUT(emittedFrameSizes.size(), std::size_t(1), 100);
    QTest::qWait(40);
    QCOMPARE(stats->openCount, 1);
    QCOMPARE(emittedFrameSizes.size(), std::size_t(1));
}

QTEST_GUILESS_MAIN(TestImageAnimationPlayer)

#include "test_imageanimationplayer.moc"
