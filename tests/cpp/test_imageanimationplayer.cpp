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
QImage frameImage(const QSize& size)
{
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return image;
}

struct FakePlaybackSourceStats
{
    int openCount = 0;
    int readCount = 0;
};

struct ManualTimerState
{
    int intervalMsec = 0;
    kiriview::RuntimeTimerCallback callback;
    bool active = false;
};

class ManualRuntimeTimer final : public kiriview::RuntimeTimerHandle
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
    void start(int intervalMsec) override
    {
        m_state->intervalMsec = intervalMsec;
        start();
    }
    void stop() override { m_state->active = false; }

private:
    std::shared_ptr<ManualTimerState> m_state;
};

class ManualTimerScheduler
{
public:
    kiriview::TimerScheduler scheduler()
    {
        return kiriview::TimerScheduler {
            []() { return qint64(0); },
            [this](QObject*, int intervalMsec, kiriview::RuntimeTimerCallback callback)
                -> std::unique_ptr<kiriview::RuntimeTimerHandle> {
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

kiriview::ImageAnimationPlaybackOpenResult openResult(
    QSize firstFrameSize, int loopCount, bool sourceHasMoreFrames)
{
    return kiriview::ImageAnimationPlaybackOpenResult {
        kiriview::ImageAnimationPlaybackOpenStatus::Success,
        frameImage(firstFrameSize),
        0,
        loopCount,
        sourceHasMoreFrames,
        {},
    };
}

kiriview::ImageAnimationPlaybackOpenResult openError(const QString& errorString)
{
    kiriview::ImageAnimationPlaybackOpenResult result;
    result.errorString = errorString;
    return result;
}

kiriview::ImageAnimationPlaybackReadResult readFrame(QSize frameSize, bool sourceHasMoreFrames)
{
    return kiriview::ImageAnimationPlaybackReadResult {
        kiriview::ImageAnimationPlaybackReadStatus::Frame,
        kiriview::AnimationFrame {
            frameImage(frameSize),
            0,
        },
        sourceHasMoreFrames,
        {},
    };
}

kiriview::ImageAnimationPlaybackReadResult readEnd()
{
    return kiriview::ImageAnimationPlaybackReadResult {
        kiriview::ImageAnimationPlaybackReadStatus::End,
        {},
        false,
        {},
    };
}

kiriview::ImageAnimationPlaybackReadResult readError(const QString& errorString)
{
    return kiriview::ImageAnimationPlaybackReadResult {
        kiriview::ImageAnimationPlaybackReadStatus::Error,
        {},
        false,
        errorString,
    };
}

class FakePlaybackSource final : public kiriview::ImageAnimationPlaybackSource
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

    FakePlaybackSource(std::vector<kiriview::ImageAnimationPlaybackOpenResult> openResults,
        std::vector<kiriview::ImageAnimationPlaybackReadResult> readResults, bool restartable,
        std::shared_ptr<FakePlaybackSourceStats> stats)
        : m_openResults(std::move(openResults))
        , m_readResults(std::move(readResults))
        , m_restartable(restartable)
        , m_stats(std::move(stats))
    {
    }

    kiriview::ImageAnimationPlaybackOpenResult open() override
    {
        m_nextFrameIndex = 0;
        const int openIndex = m_stats->openCount++;
        if (m_openResults.empty()) {
            return openError(QStringLiteral("missing open result"));
        }

        return m_openResults.at(
            std::min<std::size_t>(static_cast<std::size_t>(openIndex), m_openResults.size() - 1));
    }

    kiriview::ImageAnimationPlaybackReadResult readNextFrame() override
    {
        ++m_stats->readCount;
        if (m_nextFrameIndex >= m_readResults.size()) {
            return readEnd();
        }

        return m_readResults.at(m_nextFrameIndex++);
    }

    bool restartable() const override { return m_restartable; }

private:
    std::vector<kiriview::ImageAnimationPlaybackOpenResult> m_openResults;
    std::vector<kiriview::ImageAnimationPlaybackReadResult> m_readResults;
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
    kiriview::ImageAnimationPlaybackRequest invalidRequest;
    QVERIFY(!invalidRequest.isValid());
    QVERIFY(kiriview::makeImageAnimationPlaybackSource(std::move(invalidRequest)) == nullptr);

    kiriview::ImageAnimationPlaybackRequest readerRequest
        = kiriview::readerAnimationPlaybackRequest(
            QByteArrayLiteral("reader-data"), QByteArrayLiteral("gif"));
    QVERIFY(readerRequest.isValid());
    const auto* readerPayload
        = std::get_if<kiriview::ReaderAnimationPlaybackRequest>(&readerRequest.payload);
    QVERIFY(readerPayload != nullptr);
    QCOMPARE(readerPayload->data, QByteArrayLiteral("reader-data"));
    QCOMPARE(readerPayload->format, QByteArrayLiteral("gif"));

    kiriview::ImageAnimationPlaybackRequest apngRequest
        = kiriview::apngAnimationPlaybackRequest(QByteArrayLiteral("apng-data"));
    QVERIFY(apngRequest.isValid());
    const auto* apngPayload
        = std::get_if<kiriview::ApngAnimationPlaybackRequest>(&apngRequest.payload);
    QVERIFY(apngPayload != nullptr);
    QCOMPARE(apngPayload->data, QByteArrayLiteral("apng-data"));

    kiriview::ImageAnimationPlaybackRequest webpRequest
        = kiriview::webpAnimationPlaybackRequest(QByteArrayLiteral("webp-data"));
    QVERIFY(webpRequest.isValid());
    const auto* webpPayload
        = std::get_if<kiriview::WebPAnimationPlaybackRequest>(&webpRequest.payload);
    QVERIFY(webpPayload != nullptr);
    QCOMPARE(webpPayload->data, QByteArrayLiteral("webp-data"));

    kiriview::ImageAnimationPlaybackRequest jxlRequest
        = kiriview::jxlAnimationPlaybackRequest(QByteArrayLiteral("jxl-data"));
    QVERIFY(jxlRequest.isValid());
    const auto* jxlPayload
        = std::get_if<kiriview::JxlAnimationPlaybackRequest>(&jxlRequest.payload);
    QVERIFY(jxlPayload != nullptr);
    QCOMPARE(jxlPayload->data, QByteArrayLiteral("jxl-data"));

    kiriview::ImageAnimationPlaybackRequest heifRequest
        = kiriview::heifSequenceAnimationPlaybackRequest(QByteArrayLiteral("heif-data"));
    QVERIFY(heifRequest.isValid());
    const auto* heifPayload
        = std::get_if<kiriview::HeifSequenceAnimationPlaybackRequest>(&heifRequest.payload);
    QVERIFY(heifPayload != nullptr);
    QCOMPARE(heifPayload->data, QByteArrayLiteral("heif-data"));
}

void TestImageAnimationPlayer::startConsumesFirstFrameAndEmitsSubsequentFrames()
{
    std::vector<QSize> emittedFrameSizes;
    QString errorString;
    ManualTimerScheduler timerScheduler;
    kiriview::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage& image) { emittedFrameSizes.push_back(image.size()); },
        [&errorString](const QString& error) { errorString = error; }, {},
        timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        QSize(1, 1), std::vector<QSize> { QSize(2, 1) }, 0, true, stats));

    QCOMPARE(stats->openCount, 1);
    QVERIFY(emittedFrameSizes.empty());
    QCOMPARE(timerScheduler.timerCount(), std::size_t(1));
    QCOMPARE(timerScheduler.intervalAt(0), kiriview::normalizedAnimationFrameDelay(0));
    QVERIFY(timerScheduler.activeAt(0));

    timerScheduler.fireAt(0);

    QCOMPARE(emittedFrameSizes.at(0), QSize(2, 1));
    QVERIFY(errorString.isEmpty());
}

void TestImageAnimationPlayer::restartableSourcesEmitFirstFrameOnLoopRestart()
{
    std::vector<QSize> emittedFrameSizes;
    ManualTimerScheduler timerScheduler;
    kiriview::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage& image) { emittedFrameSizes.push_back(image.size()); },
        [](const QString&) {}, {}, timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        QSize(1, 1), std::vector<QSize> { QSize(2, 1) }, 1, true, stats));

    QCOMPARE(timerScheduler.timerCount(), std::size_t(1));
    timerScheduler.fireAt(0);
    timerScheduler.fireAt(0);
    timerScheduler.fireAt(0);

    QCOMPARE(stats->openCount, 2);
    QCOMPARE(emittedFrameSizes.at(0), QSize(2, 1));
    QCOMPARE(emittedFrameSizes.at(1), QSize(1, 1));
    QCOMPARE(emittedFrameSizes.at(2), QSize(2, 1));
}

void TestImageAnimationPlayer::nonRestartableSourcesDoNotReopenAtSequenceEnd()
{
    std::vector<QSize> emittedFrameSizes;
    ManualTimerScheduler timerScheduler;
    kiriview::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage& image) { emittedFrameSizes.push_back(image.size()); },
        [](const QString&) {}, {}, timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        QSize(1, 1), std::vector<QSize> { QSize(2, 1) }, -1, false, stats));

    QCOMPARE(timerScheduler.timerCount(), std::size_t(1));
    timerScheduler.fireAt(0);
    timerScheduler.fireAt(0);

    QCOMPARE(stats->openCount, 1);
    QCOMPARE(emittedFrameSizes.size(), std::size_t(1));
}

void TestImageAnimationPlayer::startReportsOpenErrors()
{
    std::vector<QSize> emittedFrameSizes;
    QString errorString;
    kiriview::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage& image) { emittedFrameSizes.push_back(image.size()); },
        [&errorString](const QString& error) { errorString = error; });

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<kiriview::ImageAnimationPlaybackOpenResult> {
            openError(QStringLiteral("open failed")),
        },
        std::vector<kiriview::ImageAnimationPlaybackReadResult> {}, true, stats));

    QCOMPARE(stats->openCount, 1);
    QCOMPARE(errorString, QStringLiteral("open failed"));
    QVERIFY(emittedFrameSizes.empty());
}

void TestImageAnimationPlayer::readFrameErrorsAreReportedWithoutSequenceRestart()
{
    QString errorString;
    ManualTimerScheduler timerScheduler;
    kiriview::ImageAnimationPlayer player(
        this, [](const QImage&) {}, [&errorString](const QString& error) { errorString = error; },
        {}, timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<kiriview::ImageAnimationPlaybackOpenResult> {
            openResult(QSize(1, 1), 1, true),
        },
        std::vector<kiriview::ImageAnimationPlaybackReadResult> {
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
    kiriview::ImageAnimationPlayer player(
        this, [](const QImage&) {}, [&errorString](const QString& error) { errorString = error; },
        {}, timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<kiriview::ImageAnimationPlaybackOpenResult> {
            openResult(QSize(1, 1), -1, true),
        },
        std::vector<kiriview::ImageAnimationPlaybackReadResult> {
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
    kiriview::ImageAnimationPlayer player(
        this,
        [&emittedFrameSizes](const QImage& image) { emittedFrameSizes.push_back(image.size()); },
        [&errorString](const QString& error) { errorString = error; }, {},
        timerScheduler.scheduler());

    auto stats = std::make_shared<FakePlaybackSourceStats>();
    player.start(std::make_unique<FakePlaybackSource>(
        std::vector<kiriview::ImageAnimationPlaybackOpenResult> {
            openResult(QSize(1, 1), 1, true),
            openError(QStringLiteral("restart failed")),
        },
        std::vector<kiriview::ImageAnimationPlaybackReadResult> {
            readFrame(QSize(2, 1), false),
        },
        true, stats));

    QCOMPARE(timerScheduler.timerCount(), std::size_t(1));
    timerScheduler.fireAt(0);
    timerScheduler.fireAt(0);

    QCOMPARE(errorString, QStringLiteral("restart failed"));
    QCOMPARE(stats->openCount, 2);
    QCOMPARE(emittedFrameSizes.size(), std::size_t(1));
    QCOMPARE(emittedFrameSizes.front(), QSize(2, 1));
}

QTEST_GUILESS_MAIN(TestImageAnimationPlayer)

#include "test_imageanimationplayer.moc"
