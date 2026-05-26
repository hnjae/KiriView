// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationplayer.h"

#include <QObject>
#include <QSize>
#include <QString>
#include <QTest>
#include <algorithm>
#include <memory>
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
    int readCount = 0;
};

KiriView::ImageAnimationPlaybackOpenResult openResult(
    QSize firstFrameSize, int loopCount, bool sourceHasMoreFrames)
{
    return KiriView::ImageAnimationPlaybackOpenResult {
        KiriView::ImageAnimationPlaybackOpenStatus::Success,
        frameImage(firstFrameSize),
        0,
        loopCount,
        sourceHasMoreFrames,
        {},
    };
}

KiriView::ImageAnimationPlaybackOpenResult openError(const QString &errorString)
{
    KiriView::ImageAnimationPlaybackOpenResult result;
    result.errorString = errorString;
    return result;
}

KiriView::ImageAnimationPlaybackReadResult readFrame(QSize frameSize, bool sourceHasMoreFrames)
{
    return KiriView::ImageAnimationPlaybackReadResult {
        KiriView::ImageAnimationPlaybackReadStatus::Frame,
        KiriView::AnimationFrame {
            frameImage(frameSize),
            0,
        },
        sourceHasMoreFrames,
        {},
    };
}

KiriView::ImageAnimationPlaybackReadResult readEnd()
{
    return KiriView::ImageAnimationPlaybackReadResult {
        KiriView::ImageAnimationPlaybackReadStatus::End,
        {},
        false,
        {},
    };
}

KiriView::ImageAnimationPlaybackReadResult readError(const QString &errorString)
{
    return KiriView::ImageAnimationPlaybackReadResult {
        KiriView::ImageAnimationPlaybackReadStatus::Error,
        {},
        false,
        errorString,
    };
}

class FakePlaybackSource final : public KiriView::ImageAnimationPlaybackSource
{
public:
    FakePlaybackSource(QSize firstFrameSize, std::vector<QSize> frameSizes, int loopCount,
        bool restartable, std::shared_ptr<FakePlaybackSourceStats> stats)
        : m_openResults({ openResult(firstFrameSize, loopCount, !frameSizes.empty()) })
        , m_restartable(restartable)
        , m_stats(std::move(stats))
    {
        m_readResults.reserve(frameSizes.size());
        for (std::size_t index = 0; index < frameSizes.size(); ++index) {
            m_readResults.push_back(readFrame(frameSizes.at(index), index + 1 < frameSizes.size()));
        }
    }

    FakePlaybackSource(std::vector<KiriView::ImageAnimationPlaybackOpenResult> openResults,
        std::vector<KiriView::ImageAnimationPlaybackReadResult> readResults, bool restartable,
        std::shared_ptr<FakePlaybackSourceStats> stats)
        : m_openResults(std::move(openResults))
        , m_readResults(std::move(readResults))
        , m_restartable(restartable)
        , m_stats(std::move(stats))
    {
    }

    KiriView::ImageAnimationPlaybackOpenResult open() override
    {
        m_nextFrameIndex = 0;
        const int openIndex = m_stats->openCount++;
        if (m_openResults.empty()) {
            return openError(QStringLiteral("missing open result"));
        }

        return m_openResults.at(
            std::min<std::size_t>(static_cast<std::size_t>(openIndex), m_openResults.size() - 1));
    }

    KiriView::ImageAnimationPlaybackReadResult readNextFrame() override
    {
        ++m_stats->readCount;
        if (m_nextFrameIndex >= m_readResults.size()) {
            return readEnd();
        }

        return m_readResults.at(m_nextFrameIndex++);
    }

    bool restartable() const override { return m_restartable; }

private:
    std::vector<KiriView::ImageAnimationPlaybackOpenResult> m_openResults;
    std::vector<KiriView::ImageAnimationPlaybackReadResult> m_readResults;
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
    void startReportsOpenErrors();
    void readFrameErrorsAreReportedWithoutSequenceRestart();
    void readEndStopsOptimisticSourceWithoutError();
    void restartOpenErrorsAreReported();
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

void TestImageAnimationPlayer::startReportsOpenErrors()
{
    std::vector<QSize> emittedFrameSizes;
    QString errorString;
    KiriView::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage &image) { emittedFrameSizes.push_back(image.size()); },
        [&errorString](const QString &error) { errorString = error; });

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<KiriView::ImageAnimationPlaybackOpenResult> {
            openError(QStringLiteral("open failed")),
        },
        std::vector<KiriView::ImageAnimationPlaybackReadResult> {}, true, stats));

    QCOMPARE(stats->openCount, 1);
    QCOMPARE(errorString, QStringLiteral("open failed"));
    QVERIFY(emittedFrameSizes.empty());
}

void TestImageAnimationPlayer::readFrameErrorsAreReportedWithoutSequenceRestart()
{
    QString errorString;
    KiriView::ImageAnimationPlayer player(
        this, [](const QImage &) {}, [&errorString](const QString &error) { errorString = error; });

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<KiriView::ImageAnimationPlaybackOpenResult> {
            openResult(QSize(1, 1), 1, true),
        },
        std::vector<KiriView::ImageAnimationPlaybackReadResult> {
            readError(QStringLiteral("read failed")),
        },
        true, stats));

    QTRY_COMPARE_WITH_TIMEOUT(errorString, QStringLiteral("read failed"), 100);
    QCOMPARE(stats->openCount, 1);
    QCOMPARE(stats->readCount, 1);
}

void TestImageAnimationPlayer::readEndStopsOptimisticSourceWithoutError()
{
    QString errorString;
    KiriView::ImageAnimationPlayer player(
        this, [](const QImage &) {}, [&errorString](const QString &error) { errorString = error; });

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<KiriView::ImageAnimationPlaybackOpenResult> {
            openResult(QSize(1, 1), -1, true),
        },
        std::vector<KiriView::ImageAnimationPlaybackReadResult> {
            readEnd(),
        },
        false, stats));

    QTRY_COMPARE_WITH_TIMEOUT(stats->readCount, 1, 100);
    QTest::qWait(40);
    QCOMPARE(stats->openCount, 1);
    QCOMPARE(stats->readCount, 1);
    QVERIFY(errorString.isEmpty());
}

void TestImageAnimationPlayer::restartOpenErrorsAreReported()
{
    std::vector<QSize> emittedFrameSizes;
    QString errorString;
    KiriView::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage &image) { emittedFrameSizes.push_back(image.size()); },
        [&errorString](const QString &error) { errorString = error; });

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<KiriView::ImageAnimationPlaybackOpenResult> {
            openResult(QSize(1, 1), 1, true),
            openError(QStringLiteral("restart failed")),
        },
        std::vector<KiriView::ImageAnimationPlaybackReadResult> {
            readFrame(QSize(2, 1), false),
        },
        true, stats));

    QTRY_COMPARE_WITH_TIMEOUT(errorString, QStringLiteral("restart failed"), 200);
    QCOMPARE(stats->openCount, 2);
    QCOMPARE(emittedFrameSizes.size(), std::size_t(1));
    QCOMPARE(emittedFrameSizes.front(), QSize(2, 1));
}

QTEST_GUILESS_MAIN(TestImageAnimationPlayer)

#include "test_imageanimationplayer.moc"
