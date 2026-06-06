// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageanimationplayer.h"

#include "async/timerscheduler.h"

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

struct ManualTimerState {
    int intervalMsec = 0;
    KiriView::RuntimeTimerCallback callback;
    bool active = false;
};

class ManualRuntimeTimer final : public KiriView::RuntimeTimerHandle
{
public:
    explicit ManualRuntimeTimer(std::shared_ptr<ManualTimerState> state)
        : m_state(std::move(state))
    {
    }

    ~ManualRuntimeTimer() override
    {
        if (m_state) {
            m_state->active = false;
        }
    }

    void start() override { m_state->active = true; }
    void stop() override { m_state->active = false; }

private:
    std::shared_ptr<ManualTimerState> m_state;
};

class ManualTimerScheduler
{
public:
    KiriView::TimerScheduler scheduler()
    {
        return KiriView::TimerScheduler {
            []() { return qint64(0); },
            [this](QObject *, int intervalMsec, KiriView::RuntimeTimerCallback callback)
                -> std::unique_ptr<KiriView::RuntimeTimerHandle> {
                auto state = std::make_shared<ManualTimerState>(
                    ManualTimerState { intervalMsec, std::move(callback), false });
                m_timers.push_back(state);
                return std::make_unique<ManualRuntimeTimer>(std::move(state));
            },
        };
    }

    std::size_t timerCount() const { return m_timers.size(); }
    int intervalAt(std::size_t index) const { return m_timers.at(index)->intervalMsec; }
    bool activeAt(std::size_t index) const { return m_timers.at(index)->active; }

    void fireAt(std::size_t index)
    {
        std::shared_ptr<ManualTimerState> state = m_timers.at(index);
        if (!state->active || !state->callback) {
            return;
        }

        state->active = false;
        state->callback();
    }

private:
    std::vector<std::shared_ptr<ManualTimerState>> m_timers;
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
    void playbackRequestsAreTypedValues();
    void startConsumesFirstFrameAndEmitsSubsequentFrames();
    void restartableSourcesEmitFirstFrameOnLoopRestart();
    void nonRestartableSourcesDoNotReopenAtSequenceEnd();
    void startReportsOpenErrors();
    void readFrameErrorsAreReportedWithoutSequenceRestart();
    void readEndStopsOptimisticSourceWithoutError();
    void restartOpenErrorsAreReported();
};

void TestImageAnimationPlayer::playbackRequestsAreTypedValues()
{
    KiriView::ImageAnimationPlaybackRequest invalidRequest;
    QVERIFY(!invalidRequest.isValid());
    QVERIFY(KiriView::makeImageAnimationPlaybackSource(std::move(invalidRequest)) == nullptr);

    KiriView::ImageAnimationPlaybackRequest readerRequest
        = KiriView::readerAnimationPlaybackRequest(
            QByteArrayLiteral("reader-data"), QByteArrayLiteral("gif"));
    QVERIFY(readerRequest.isValid());
    const auto *readerPayload
        = std::get_if<KiriView::ReaderAnimationPlaybackRequest>(&readerRequest.payload);
    QVERIFY(readerPayload != nullptr);
    QCOMPARE(readerPayload->data, QByteArrayLiteral("reader-data"));
    QCOMPARE(readerPayload->format, QByteArrayLiteral("gif"));

    KiriView::ImageAnimationPlaybackRequest apngRequest
        = KiriView::apngAnimationPlaybackRequest(QByteArrayLiteral("apng-data"));
    QVERIFY(apngRequest.isValid());
    const auto *apngPayload
        = std::get_if<KiriView::ApngAnimationPlaybackRequest>(&apngRequest.payload);
    QVERIFY(apngPayload != nullptr);
    QCOMPARE(apngPayload->data, QByteArrayLiteral("apng-data"));

    KiriView::ImageAnimationPlaybackRequest heifRequest
        = KiriView::heifSequenceAnimationPlaybackRequest(QByteArrayLiteral("heif-data"));
    QVERIFY(heifRequest.isValid());
    const auto *heifPayload
        = std::get_if<KiriView::HeifSequenceAnimationPlaybackRequest>(&heifRequest.payload);
    QVERIFY(heifPayload != nullptr);
    QCOMPARE(heifPayload->data, QByteArrayLiteral("heif-data"));
}

void TestImageAnimationPlayer::startConsumesFirstFrameAndEmitsSubsequentFrames()
{
    std::vector<QSize> emittedFrameSizes;
    QString errorString;
    ManualTimerScheduler timerScheduler;
    KiriView::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage &image) { emittedFrameSizes.push_back(image.size()); },
        [&errorString](const QString &error) { errorString = error; }, {},
        timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        QSize(1, 1), std::vector<QSize> { QSize(2, 1) }, 0, true, stats));

    QCOMPARE(stats->openCount, 1);
    QVERIFY(emittedFrameSizes.empty());
    QCOMPARE(timerScheduler.timerCount(), std::size_t(1));
    QCOMPARE(timerScheduler.intervalAt(0), KiriView::normalizedAnimationFrameDelay(0));
    QVERIFY(timerScheduler.activeAt(0));

    timerScheduler.fireAt(0);

    QCOMPARE(emittedFrameSizes.at(0), QSize(2, 1));
    QVERIFY(errorString.isEmpty());
}

void TestImageAnimationPlayer::restartableSourcesEmitFirstFrameOnLoopRestart()
{
    std::vector<QSize> emittedFrameSizes;
    ManualTimerScheduler timerScheduler;
    KiriView::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage &image) { emittedFrameSizes.push_back(image.size()); },
        [](const QString &) {}, {}, timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        QSize(1, 1), std::vector<QSize> { QSize(2, 1) }, 1, true, stats));

    timerScheduler.fireAt(0);
    timerScheduler.fireAt(1);
    timerScheduler.fireAt(2);

    QCOMPARE(stats->openCount, 2);
    QCOMPARE(emittedFrameSizes.at(0), QSize(2, 1));
    QCOMPARE(emittedFrameSizes.at(1), QSize(1, 1));
    QCOMPARE(emittedFrameSizes.at(2), QSize(2, 1));
}

void TestImageAnimationPlayer::nonRestartableSourcesDoNotReopenAtSequenceEnd()
{
    std::vector<QSize> emittedFrameSizes;
    ManualTimerScheduler timerScheduler;
    KiriView::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage &image) { emittedFrameSizes.push_back(image.size()); },
        [](const QString &) {}, {}, timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        QSize(1, 1), std::vector<QSize> { QSize(2, 1) }, -1, false, stats));

    timerScheduler.fireAt(0);
    timerScheduler.fireAt(1);

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
    ManualTimerScheduler timerScheduler;
    KiriView::ImageAnimationPlayer player(
        this, [](const QImage &) {}, [&errorString](const QString &error) { errorString = error; },
        {}, timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<KiriView::ImageAnimationPlaybackOpenResult> {
            openResult(QSize(1, 1), 1, true),
        },
        std::vector<KiriView::ImageAnimationPlaybackReadResult> {
            readError(QStringLiteral("read failed")),
        },
        true, stats));

    timerScheduler.fireAt(0);

    QCOMPARE(errorString, QStringLiteral("read failed"));
    QCOMPARE(stats->openCount, 1);
    QCOMPARE(stats->readCount, 1);
}

void TestImageAnimationPlayer::readEndStopsOptimisticSourceWithoutError()
{
    QString errorString;
    ManualTimerScheduler timerScheduler;
    KiriView::ImageAnimationPlayer player(
        this, [](const QImage &) {}, [&errorString](const QString &error) { errorString = error; },
        {}, timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<KiriView::ImageAnimationPlaybackOpenResult> {
            openResult(QSize(1, 1), -1, true),
        },
        std::vector<KiriView::ImageAnimationPlaybackReadResult> {
            readEnd(),
        },
        false, stats));

    timerScheduler.fireAt(0);

    QCOMPARE(stats->openCount, 1);
    QCOMPARE(stats->readCount, 1);
    QVERIFY(errorString.isEmpty());
}

void TestImageAnimationPlayer::restartOpenErrorsAreReported()
{
    std::vector<QSize> emittedFrameSizes;
    QString errorString;
    ManualTimerScheduler timerScheduler;
    KiriView::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage &image) { emittedFrameSizes.push_back(image.size()); },
        [&errorString](const QString &error) { errorString = error; }, {},
        timerScheduler.scheduler());

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

    timerScheduler.fireAt(0);
    timerScheduler.fireAt(1);

    QCOMPARE(errorString, QStringLiteral("restart failed"));
    QCOMPARE(stats->openCount, 2);
    QCOMPARE(emittedFrameSizes.size(), std::size_t(1));
    QCOMPARE(emittedFrameSizes.front(), QSize(2, 1));
}

QTEST_GUILESS_MAIN(TestImageAnimationPlayer)

#include "test_imageanimationplayer.moc"
