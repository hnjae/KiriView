// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videosourceloadruntime.h"

#include <QObject>
#include <QStringList>
#include <QTest>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

class TestVideoSourceLoadRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptySourceClearsPlaybackAndPublicState();
    void sourceLoadStartsResolverAfterClearingPlayback();
    void acceptedResolverCompletionPublishesPlaybackUrl();
    void staleResolverCompletionsAreIgnored();
    void acceptedResolverFailurePublishesError();
    void shutdownCancelsAndCleansUpOnce();
};

namespace {
template <typename> inline constexpr bool alwaysFalse = false;

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

struct SourceLoadFixture {
    QObject receiver;
    std::shared_ptr<FakeResolverState> resolverState = std::make_shared<FakeResolverState>();
    KiriView::VideoSourceLoadRuntime runtime { std::make_unique<FakeVideoPlaybackUrlResolver>(
        resolverState) };
    QStringList events;
    QUrl readySourceUrl;
    QUrl readyPlaybackUrl;
    QUrl failedSourceUrl;
    QString failedErrorString;
    QString failedDiagnosticDetail;
    KiriView::VideoSourceLoadFailureKind failedKind
        = KiriView::VideoSourceLoadFailureKind::PlaybackUrlResolution;

    KiriView::VideoSourceLoadPlanCallback planCallback()
    {
        return [this](KiriView::VideoSourceLoadPlan plan) {
            for (const KiriView::VideoSourceLoadOperation &operation : plan) {
                recordOperation(operation);
            }
        };
    }

    void recordOperation(const KiriView::VideoSourceLoadOperation &operation)
    {
        std::visit(
            [this](const auto &payload) {
                using Operation = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<Operation,
                                  KiriView::ClearVideoPlaybackSourceOperation>) {
                    events.push_back(QStringLiteral("clear-playback"));
                } else if constexpr (std::is_same_v<Operation,
                                         KiriView::ResetClearedVideoSourceOperation>) {
                    events.push_back(QStringLiteral("source-cleared"));
                } else if constexpr (std::is_same_v<Operation,
                                         KiriView::ResetVideoSourceLoadOperation>) {
                    events.push_back(
                        QStringLiteral("source-load-started:%1").arg(payload.sourceUrl.toString()));
                } else if constexpr (std::is_same_v<Operation,
                                         KiriView::ApplyVideoPlaybackUrlOperation>) {
                    events.push_back(QStringLiteral("playback-ready"));
                    readySourceUrl = payload.sourceUrl;
                    readyPlaybackUrl = payload.playbackUrl;
                } else if constexpr (std::is_same_v<Operation,
                                         KiriView::PublishVideoSourceLoadFailureOperation>) {
                    events.push_back(QStringLiteral("source-load-failed"));
                    failedSourceUrl = payload.failure.sourceUrl;
                    failedErrorString = payload.failure.userMessage;
                    failedDiagnosticDetail = payload.failure.diagnosticDetail;
                    failedKind = payload.failure.kind;
                } else {
                    static_assert(alwaysFalse<Operation>, "Unhandled video source load operation");
                }
            },
            operation);
    }

    void setSourceUrl(const QUrl &sourceUrl)
    {
        runtime.setSourceUrl(sourceUrl, &receiver, planCallback());
    }
};
}

void TestVideoSourceLoadRuntime::emptySourceClearsPlaybackAndPublicState()
{
    SourceLoadFixture fixture;

    fixture.setSourceUrl(QUrl());

    QCOMPARE(fixture.resolverState->cancelCount, 1);
    QCOMPARE(fixture.resolverState->cleanupCount, 1);
    QVERIFY(!fixture.runtime.active());
    QVERIFY(fixture.resolverState->requests.empty());
    QCOMPARE(fixture.events,
        QStringList({ QStringLiteral("clear-playback"), QStringLiteral("source-cleared") }));
}

void TestVideoSourceLoadRuntime::sourceLoadStartsResolverAfterClearingPlayback()
{
    SourceLoadFixture fixture;
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));

    fixture.setSourceUrl(sourceUrl);

    QCOMPARE(fixture.resolverState->cancelCount, 1);
    QCOMPARE(fixture.resolverState->cleanupCount, 1);
    QVERIFY(fixture.runtime.active());
    QCOMPARE(fixture.resolverState->requests.size(), std::size_t(1));
    QCOMPARE(fixture.resolverState->requests.back().sourceUrl, sourceUrl);
    QVERIFY(fixture.resolverState->requests.back().operationId != 0);
    QCOMPARE(fixture.events,
        QStringList({ QStringLiteral("clear-playback"),
            QStringLiteral("source-load-started:zip:///home/me/videos.zip!/clip.mp4") }));
}

void TestVideoSourceLoadRuntime::acceptedResolverCompletionPublishesPlaybackUrl()
{
    SourceLoadFixture fixture;
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));
    const QUrl playbackUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/clip.mp4"));

    fixture.setSourceUrl(sourceUrl);
    auto &request = fixture.resolverState->requests.back();
    request.resolvedCallback(KiriView::VideoPlaybackUrlResolution {
        request.operationId, request.sourceUrl, playbackUrl });

    QVERIFY(!fixture.runtime.active());
    QCOMPARE(fixture.readySourceUrl, sourceUrl);
    QCOMPARE(fixture.readyPlaybackUrl, playbackUrl);
    QCOMPARE(fixture.events.back(), QStringLiteral("playback-ready"));
}

void TestVideoSourceLoadRuntime::staleResolverCompletionsAreIgnored()
{
    SourceLoadFixture fixture;
    const QUrl firstSourceUrl(QStringLiteral("zip:///home/me/videos.zip!/first.mp4"));
    const QUrl secondSourceUrl(QStringLiteral("zip:///home/me/videos.zip!/second.mp4"));
    const QUrl stalePlaybackUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/first.mp4"));
    const QUrl currentPlaybackUrl = QUrl::fromLocalFile(QStringLiteral("/tmp/second.mp4"));

    fixture.setSourceUrl(firstSourceUrl);
    fixture.setSourceUrl(secondSourceUrl);

    auto &firstRequest = fixture.resolverState->requests.front();
    firstRequest.resolvedCallback(KiriView::VideoPlaybackUrlResolution {
        firstRequest.operationId,
        firstRequest.sourceUrl,
        stalePlaybackUrl,
    });
    QVERIFY(fixture.readyPlaybackUrl.isEmpty());
    QVERIFY(fixture.runtime.active());

    auto &secondRequest = fixture.resolverState->requests.back();
    secondRequest.resolvedCallback(KiriView::VideoPlaybackUrlResolution {
        secondRequest.operationId,
        secondRequest.sourceUrl,
        currentPlaybackUrl,
    });
    QCOMPARE(fixture.readySourceUrl, secondSourceUrl);
    QCOMPARE(fixture.readyPlaybackUrl, currentPlaybackUrl);
    QVERIFY(!fixture.runtime.active());
}

void TestVideoSourceLoadRuntime::acceptedResolverFailurePublishesError()
{
    SourceLoadFixture fixture;
    const QUrl sourceUrl(QStringLiteral("zip:///home/me/videos.zip!/clip.mp4"));

    fixture.setSourceUrl(sourceUrl);
    auto &request = fixture.resolverState->requests.back();
    request.failedCallback(
        request.operationId, request.sourceUrl, QStringLiteral("resolution failed"));

    QVERIFY(!fixture.runtime.active());
    QCOMPARE(fixture.failedSourceUrl, sourceUrl);
    QVERIFY(fixture.failedKind == KiriView::VideoSourceLoadFailureKind::PlaybackUrlResolution);
    QCOMPARE(fixture.failedErrorString, QStringLiteral("resolution failed"));
    QCOMPARE(fixture.failedDiagnosticDetail, QStringLiteral("resolution failed"));
    QCOMPARE(fixture.events.back(), QStringLiteral("source-load-failed"));
}

void TestVideoSourceLoadRuntime::shutdownCancelsAndCleansUpOnce()
{
    auto resolverState = std::make_shared<FakeResolverState>();
    {
        KiriView::VideoSourceLoadRuntime runtime(
            std::make_unique<FakeVideoPlaybackUrlResolver>(resolverState));
        runtime.shutdown();
        runtime.shutdown();
        QCOMPARE(resolverState->cancelCount, 1);
        QCOMPARE(resolverState->cleanupCount, 1);
    }

    QCOMPARE(resolverState->cancelCount, 1);
    QCOMPARE(resolverState->cleanupCount, 1);
}

QTEST_GUILESS_MAIN(TestVideoSourceLoadRuntime)

#include "test_videosourceloadruntime.moc"
