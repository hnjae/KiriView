// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videodocumentruntime.h"

#include <QObject>
#include <QPointer>
#include <QTest>
#include <memory>
#include <utility>
#include <vector>

class TestVideoDocumentRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialStateIsNull();
    void mediaBackendFactoryIsLazyUntilPlaybackUrlResolution();
    void localResolverCompletionUsesOriginalPlaybackUrl();
    void settingAndClearingSourcePreservesUserFacingUrlAndTitle();
    void resolverCanReturnSeparatePlaybackUrl();
    void resolverFailureSurfacesErrorWithoutChangingSourceUrl();
    void staleResolverCompletionsAreIgnored();
    void resolverCleanupRunsOnSourceChangeAndDestruction();
    void seekByClampsToKnownDuration();
    void seekByNoopsWhenNotSeekable();
    void videoOutputDetachAndDestructionClearBackendOutput();
};

namespace {
class FakeVideoMediaBackend final : public KiriView::VideoMediaBackend
{
public:
    void setCallbacks(KiriView::VideoMediaBackendCallbacks nextCallbacks) override
    {
        callbacks = std::move(nextCallbacks);
    }

    void setSource(const QUrl &nextSourceUrl) override
    {
        sourceUrl = nextSourceUrl;
        ++setSourceCount;
    }

    void play() override
    {
        isPlaying = true;
        ++playCount;
        callbacks.playingChanged();
    }

    void pause() override
    {
        isPlaying = false;
        ++pauseCount;
        callbacks.playingChanged();
    }

    void stop() override
    {
        isPlaying = false;
        ++stopCount;
        callbacks.playingChanged();
    }

    void setPosition(qint64 nextPosition) override
    {
        currentPosition = nextPosition;
        ++setPositionCount;
        callbacks.positionChanged();
    }

    void setVideoOutput(QObject *nextVideoOutput) override
    {
        output = nextVideoOutput;
        ++setVideoOutputCount;
    }

    QObject *videoOutput() const override { return output.data(); }
    KiriView::VideoDocumentStatus mediaStatus() const override { return currentStatus; }
    QString errorString() const override { return currentErrorString; }
    qint64 duration() const override { return currentDuration; }
    qint64 position() const override { return currentPosition; }
    bool playing() const override { return isPlaying; }
    bool seekable() const override { return isSeekable; }
    bool hasVideo() const override { return videoAvailable; }
    bool hasAudio() const override { return audioAvailable; }

    void emitStatus(KiriView::VideoDocumentStatus status)
    {
        currentStatus = status;
        callbacks.mediaStatusChanged();
    }

    void emitDuration(qint64 duration)
    {
        currentDuration = duration;
        callbacks.durationChanged();
    }

    void emitPosition(qint64 position)
    {
        currentPosition = position;
        callbacks.positionChanged();
    }

    void emitSeekable(bool seekable)
    {
        isSeekable = seekable;
        callbacks.seekableChanged();
    }

    QUrl sourceUrl;
    QPointer<QObject> output;
    KiriView::VideoMediaBackendCallbacks callbacks;
    KiriView::VideoDocumentStatus currentStatus = KiriView::VideoDocumentStatus::Null;
    QString currentErrorString;
    qint64 currentDuration = 0;
    qint64 currentPosition = 0;
    bool isPlaying = false;
    bool isSeekable = false;
    bool videoAvailable = false;
    bool audioAvailable = false;
    int setSourceCount = 0;
    int setPositionCount = 0;
    int setVideoOutputCount = 0;
    int playCount = 0;
    int pauseCount = 0;
    int stopCount = 0;
};

struct FakeResolverState {
    struct Request {
        quint64 operationId = 0;
        QUrl sourceUrl;
        KiriView::VideoPlaybackUrlResolvedCallback resolvedCallback;
        KiriView::VideoPlaybackUrlFailedCallback failedCallback;
    };

    std::vector<Request> requests;
    int cancelCount = 0;
    int cleanupCount = 0;
};

class FakeVideoPlaybackUrlResolver final : public KiriView::VideoPlaybackUrlResolver
{
public:
    explicit FakeVideoPlaybackUrlResolver(std::shared_ptr<FakeResolverState> resolverState)
        : state(std::move(resolverState))
    {
    }

    void resolve(quint64 operationId, const QUrl &sourceUrl, QObject *,
        KiriView::VideoPlaybackUrlResolvedCallback resolvedCallback,
        KiriView::VideoPlaybackUrlFailedCallback failedCallback) override
    {
        state->requests.push_back(FakeResolverState::Request {
            operationId, sourceUrl, std::move(resolvedCallback), std::move(failedCallback) });
    }

    void cancel() override { ++state->cancelCount; }
    void cleanup() override { ++state->cleanupCount; }

private:
    std::shared_ptr<FakeResolverState> state;
};

struct RuntimeFixture {
    QObject documentObject;
    FakeVideoMediaBackend *backend = nullptr;
    std::shared_ptr<FakeResolverState> resolverState = std::make_shared<FakeResolverState>();
    std::vector<KiriView::VideoDocumentChange> changes;
    std::unique_ptr<KiriView::VideoDocumentRuntime> runtime;

    RuntimeFixture()
    {
        auto mediaBackend = std::make_unique<FakeVideoMediaBackend>();
        backend = mediaBackend.get();
        runtime = std::make_unique<KiriView::VideoDocumentRuntime>(
            &documentObject,
            [this](const std::vector<KiriView::VideoDocumentChange> &nextChanges) {
                changes.insert(changes.end(), nextChanges.begin(), nextChanges.end());
            },
            std::move(mediaBackend), std::make_unique<FakeVideoPlaybackUrlResolver>(resolverState));
    }

    void resolveLatest(const QUrl &playbackUrl)
    {
        auto &request = resolverState->requests.back();
        request.resolvedCallback(KiriView::VideoPlaybackUrlResolution {
            request.operationId,
            request.sourceUrl,
            playbackUrl,
        });
    }

    void failLatest(const QString &errorString)
    {
        auto &request = resolverState->requests.back();
        request.failedCallback(request.operationId, request.sourceUrl, errorString);
    }
};
}

void TestVideoDocumentRuntime::initialStateIsNull()
{
    RuntimeFixture fixture;

    QCOMPARE(fixture.runtime->sourceUrl(), QUrl());
    QCOMPARE(fixture.runtime->status(), KiriView::VideoDocumentStatus::Null);
    QCOMPARE(fixture.runtime->errorString(), QString());
    QCOMPARE(fixture.runtime->windowTitleFileName(), QString());
    QCOMPARE(fixture.runtime->duration(), 0);
    QCOMPARE(fixture.runtime->position(), 0);
    QVERIFY(!fixture.runtime->playing());
    QVERIFY(!fixture.runtime->seekable());
    QVERIFY(!fixture.runtime->hasVideo());
    QVERIFY(!fixture.runtime->hasAudio());
    QCOMPARE(fixture.runtime->videoOutput(), nullptr);
}

void TestVideoDocumentRuntime::mediaBackendFactoryIsLazyUntilPlaybackUrlResolution()
{
    QObject documentObject;
    QObject output;
    auto resolverState = std::make_shared<FakeResolverState>();
    FakeVideoMediaBackend *backend = nullptr;
    QObject *factoryParent = nullptr;
    int factoryCallCount = 0;
    KiriView::VideoDocumentRuntime runtime(&documentObject, {},
        std::unique_ptr<KiriView::VideoMediaBackend>(),
        std::make_unique<FakeVideoPlaybackUrlResolver>(resolverState), [&](QObject *parent) {
            factoryParent = parent;
            ++factoryCallCount;
            auto mediaBackend = std::make_unique<FakeVideoMediaBackend>();
            backend = mediaBackend.get();
            return mediaBackend;
        });
    const QUrl sourceUrl = QUrl::fromLocalFile(QStringLiteral("/home/me/clip.mp4"));

    QCOMPARE(factoryCallCount, 0);
    runtime.setVideoOutput(&output);
    QCOMPARE(factoryCallCount, 0);

    runtime.setSourceUrl(sourceUrl);
    QCOMPARE(factoryCallCount, 0);
    QCOMPARE(runtime.status(), KiriView::VideoDocumentStatus::Loading);
    QCOMPARE(resolverState->requests.size(), std::size_t(1));

    auto &request = resolverState->requests.back();
    request.resolvedCallback(KiriView::VideoPlaybackUrlResolution {
        request.operationId,
        request.sourceUrl,
        sourceUrl,
    });

    QCOMPARE(factoryCallCount, 1);
    QCOMPARE(factoryParent, &documentObject);
    QVERIFY(backend != nullptr);
    QCOMPARE(backend->sourceUrl, sourceUrl);
    QCOMPARE(backend->videoOutput(), &output);
}

void TestVideoDocumentRuntime::localResolverCompletionUsesOriginalPlaybackUrl()
{
    RuntimeFixture fixture;
    const QUrl sourceUrl = QUrl::fromLocalFile(QStringLiteral("/home/me/clip.mp4"));

    fixture.runtime->setSourceUrl(sourceUrl);
    fixture.resolveLatest(sourceUrl);

    QCOMPARE(fixture.runtime->sourceUrl(), sourceUrl);
    QCOMPARE(fixture.backend->sourceUrl, sourceUrl);
}

void TestVideoDocumentRuntime::settingAndClearingSourcePreservesUserFacingUrlAndTitle()
{
    RuntimeFixture fixture;
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/chapter/clip.mov"));

    fixture.runtime->setSourceUrl(sourceUrl);
    QCOMPARE(fixture.runtime->sourceUrl(), sourceUrl);
    QCOMPARE(fixture.runtime->windowTitleFileName(), QStringLiteral("clip.mov"));
    QCOMPARE(fixture.runtime->status(), KiriView::VideoDocumentStatus::Loading);
    QCOMPARE(fixture.resolverState->requests.size(), std::size_t(1));

    const QUrl playbackUrl
        = QUrl::fromLocalFile(QStringLiteral("/run/user/1000/kio-fuse/clip.mov"));
    fixture.resolveLatest(playbackUrl);
    QCOMPARE(fixture.runtime->sourceUrl(), sourceUrl);
    QCOMPARE(fixture.backend->sourceUrl, playbackUrl);

    fixture.backend->emitStatus(KiriView::VideoDocumentStatus::Ready);
    QCOMPARE(fixture.runtime->status(), KiriView::VideoDocumentStatus::Ready);

    fixture.runtime->setSourceUrl(QUrl());
    QCOMPARE(fixture.runtime->sourceUrl(), QUrl());
    QCOMPARE(fixture.runtime->windowTitleFileName(), QString());
    QCOMPARE(fixture.runtime->status(), KiriView::VideoDocumentStatus::Null);
    QCOMPARE(fixture.backend->sourceUrl, QUrl());
}

void TestVideoDocumentRuntime::resolverCanReturnSeparatePlaybackUrl()
{
    RuntimeFixture fixture;
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));
    const QUrl playbackUrl
        = QUrl::fromLocalFile(QStringLiteral("/run/user/1000/kio-fuse/clip.mp4"));

    fixture.runtime->setSourceUrl(sourceUrl);
    fixture.resolveLatest(playbackUrl);

    QCOMPARE(fixture.runtime->sourceUrl(), sourceUrl);
    QCOMPARE(fixture.backend->sourceUrl, playbackUrl);
}

void TestVideoDocumentRuntime::resolverFailureSurfacesErrorWithoutChangingSourceUrl()
{
    RuntimeFixture fixture;
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));

    fixture.runtime->setSourceUrl(sourceUrl);
    fixture.failLatest(QStringLiteral("resolution failed"));

    QCOMPARE(fixture.runtime->sourceUrl(), sourceUrl);
    QCOMPARE(fixture.runtime->status(), KiriView::VideoDocumentStatus::Error);
    QCOMPARE(fixture.runtime->errorString(), QStringLiteral("resolution failed"));
    QCOMPARE(fixture.backend->sourceUrl, QUrl());
}

void TestVideoDocumentRuntime::staleResolverCompletionsAreIgnored()
{
    RuntimeFixture fixture;
    const QUrl firstSourceUrl(QStringLiteral("zip:///home/me/videos.zip!/first.mp4"));
    const QUrl secondSourceUrl(QStringLiteral("zip:///home/me/videos.zip!/second.mp4"));
    const QUrl stalePlaybackUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/first.mp4"));
    const QUrl currentPlaybackUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/second.mp4"));

    fixture.runtime->setSourceUrl(firstSourceUrl);
    fixture.runtime->setSourceUrl(secondSourceUrl);

    auto &firstRequest = fixture.resolverState->requests.front();
    firstRequest.resolvedCallback(KiriView::VideoPlaybackUrlResolution {
        firstRequest.operationId,
        firstRequest.sourceUrl,
        stalePlaybackUrl,
    });
    QCOMPARE(fixture.runtime->sourceUrl(), secondSourceUrl);
    QCOMPARE(fixture.backend->sourceUrl, QUrl());

    fixture.resolveLatest(currentPlaybackUrl);
    QCOMPARE(fixture.runtime->sourceUrl(), secondSourceUrl);
    QCOMPARE(fixture.backend->sourceUrl, currentPlaybackUrl);
}

void TestVideoDocumentRuntime::resolverCleanupRunsOnSourceChangeAndDestruction()
{
    auto resolverState = std::make_shared<FakeResolverState>();
    {
        QObject documentObject;
        auto mediaBackend = std::make_unique<FakeVideoMediaBackend>();
        KiriView::VideoDocumentRuntime runtime(&documentObject, {}, std::move(mediaBackend),
            std::make_unique<FakeVideoPlaybackUrlResolver>(resolverState));

        runtime.setSourceUrl(QUrl(QStringLiteral("zip:///home/me/videos.zip!/first.mp4")));
        runtime.setSourceUrl(QUrl(QStringLiteral("zip:///home/me/videos.zip!/second.mp4")));
        QCOMPARE(resolverState->cancelCount, 2);
        QCOMPARE(resolverState->cleanupCount, 2);
    }

    QCOMPARE(resolverState->cancelCount, 3);
    QCOMPARE(resolverState->cleanupCount, 3);
}

void TestVideoDocumentRuntime::seekByClampsToKnownDuration()
{
    RuntimeFixture fixture;

    fixture.backend->emitDuration(10000);
    fixture.backend->emitPosition(5000);
    fixture.backend->emitSeekable(true);

    fixture.runtime->seekBy(7000);
    QCOMPARE(fixture.backend->setPositionCount, 1);
    QCOMPARE(fixture.backend->currentPosition, 10000);
    QCOMPARE(fixture.runtime->position(), 10000);

    fixture.runtime->seekBy(-20000);
    QCOMPARE(fixture.backend->setPositionCount, 2);
    QCOMPARE(fixture.backend->currentPosition, 0);
    QCOMPARE(fixture.runtime->position(), 0);
}

void TestVideoDocumentRuntime::seekByNoopsWhenNotSeekable()
{
    RuntimeFixture fixture;

    fixture.backend->emitDuration(10000);
    fixture.backend->emitPosition(5000);
    fixture.backend->emitSeekable(false);

    fixture.runtime->seekBy(5000);
    QCOMPARE(fixture.backend->setPositionCount, 0);
    QCOMPARE(fixture.runtime->position(), 5000);
    QCOMPARE(KiriView::VideoDocumentRuntime::clampedSeekPosition(5000, 5000, 10000, false), 5000);
}

void TestVideoDocumentRuntime::videoOutputDetachAndDestructionClearBackendOutput()
{
    RuntimeFixture fixture;
    auto *output = new QObject();

    fixture.runtime->setVideoOutput(output);
    QCOMPARE(fixture.runtime->videoOutput(), output);
    QCOMPARE(fixture.backend->videoOutput(), output);

    fixture.runtime->setVideoOutput(nullptr);
    QCOMPARE(fixture.runtime->videoOutput(), nullptr);
    QCOMPARE(fixture.backend->videoOutput(), nullptr);

    output = new QObject();
    fixture.runtime->setVideoOutput(output);
    delete output;

    QCOMPARE(fixture.runtime->videoOutput(), nullptr);
    QCOMPARE(fixture.backend->videoOutput(), nullptr);
}

QTEST_GUILESS_MAIN(TestVideoDocumentRuntime)

#include "test_videodocumentruntime.moc"
