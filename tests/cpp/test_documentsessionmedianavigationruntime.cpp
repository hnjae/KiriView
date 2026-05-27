// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmedianavigationruntime.h"

#include "image_async_test_support.h"
#include "location/imageurl.h"

#include <QObject>
#include <QTest>
#include <memory>
#include <utility>
#include <vector>

class TestDocumentSessionMediaNavigationRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void invalidScopeCompletesWithoutStartingProvider();
    void successfulLoadPublishesCandidates();
    void refreshPublishesBoundaryPlanAndCandidates();
    void openPublishesTargetPlanAndCandidates();
    void failedOpenPublishesUnknownResult();
    void cancelRejectsLateCompletion();
    void cancelRejectsLateError();
    void predicateRejectionDropsCurrentCompletion();
    void acceptedScopeAllowsEquivalentCursorConfirmation();
};

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::MediaNavigationCandidate mediaCandidate(const QUrl &url)
{
    return KiriView::MediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

struct ManualMediaCandidateLoad {
    QObject *object = nullptr;
    QUrl parentUrl;
    KiriView::MediaCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualMediaCandidateProvider
{
public:
    KiriView::MediaNavigationCandidateProvider provider()
    {
        return KiriView::MediaNavigationCandidateProvider {
            [this](QObject *receiver, QUrl parentUrl, KiriView::MediaCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                auto load = std::make_shared<ManualMediaCandidateLoad>();
                load->parentUrl = std::move(parentUrl);
                load->callback = std::move(callback);
                load->errorCallback = std::move(errorCallback);

                KiriView::ImageIoJob job
                    = KiriView::TestSupport::Detail::startManualIoJob(receiver, load);
                m_loads.push_back(load);
                return job;
            },
        };
    }

    std::size_t loadCount() const { return m_loads.size(); }

    ManualMediaCandidateLoad &loadAt(std::size_t index) { return *m_loads.at(index); }

    void deliverIgnoringCancellation(
        std::size_t index, std::vector<KiriView::MediaNavigationCandidate> candidates)
    {
        ManualMediaCandidateLoad &load = loadAt(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

    void deliverErrorIgnoringCancellation(std::size_t index, const QString &errorString)
    {
        ManualMediaCandidateLoad &load = loadAt(index);
        if (load.errorCallback) {
            load.errorCallback(errorString);
        }
    }

private:
    std::vector<std::shared_ptr<ManualMediaCandidateLoad>> m_loads;
};

struct RuntimeFixture {
    QObject receiver;
    ManualMediaCandidateProvider provider;
    KiriView::DocumentSessionMediaNavigationRuntime runtime { provider.provider() };
    int completionCount = 0;
    KiriView::DocumentSessionMediaNavigationCandidatesResult result;
    bool acceptScope = true;

    void load(const KiriView::DirectMediaScope &scope)
    {
        runtime.loadCandidates(
            &receiver, scope, [this](const KiriView::DirectMediaScope &) { return acceptScope; },
            [this](KiriView::DocumentSessionMediaNavigationCandidatesResult loadResult) {
                ++completionCount;
                result = std::move(loadResult);
            });
    }
};

KiriView::DirectMediaScope mediaScope(const QUrl &currentUrl)
{
    return KiriView::DirectMediaScope {
        currentUrl,
        localUrl(QStringLiteral("/media/")),
        7,
    };
}
}

void TestDocumentSessionMediaNavigationRuntime::invalidScopeCompletesWithoutStartingProvider()
{
    RuntimeFixture fixture;

    fixture.load(KiriView::DirectMediaScope {});

    QCOMPARE(fixture.provider.loadCount(), std::size_t(0));
    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(!fixture.result.succeeded);
    QVERIFY(fixture.result.candidates.empty());
}

void TestDocumentSessionMediaNavigationRuntime::successfulLoadPublishesCandidates()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));

    fixture.load(mediaScope(currentUrl));
    fixture.provider.deliverIgnoringCancellation(
        0, { mediaCandidate(currentUrl), mediaCandidate(nextUrl) });

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(fixture.result.succeeded);
    QCOMPARE(fixture.result.candidates.size(), std::size_t(2));
    QCOMPARE(fixture.result.candidates.at(1).url, nextUrl);
}

void TestDocumentSessionMediaNavigationRuntime::refreshPublishesBoundaryPlanAndCandidates()
{
    RuntimeFixture fixture;
    const QUrl previousUrl = localUrl(QStringLiteral("/media/00.mp4"));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    KiriView::DocumentSessionMediaNavigationRefreshResult result;

    fixture.runtime.refresh(
        &fixture.receiver, mediaScope(currentUrl),
        [](const KiriView::DirectMediaScope &) { return true; },
        [&fixture, &result](KiriView::DocumentSessionMediaNavigationRefreshResult loadResult) {
            ++fixture.completionCount;
            result = std::move(loadResult);
        });
    fixture.provider.deliverIgnoringCancellation(
        0, { mediaCandidate(previousUrl), mediaCandidate(currentUrl), mediaCandidate(nextUrl) });

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(result.succeeded);
    QCOMPARE(result.candidates.size(), std::size_t(3));
    QCOMPARE(result.boundaryState.currentNumber, 2);
    QCOMPARE(result.boundaryState.count, 3);
    QVERIFY(result.boundaryState.canOpenPrevious);
    QVERIFY(result.boundaryState.canOpenNext);
}

void TestDocumentSessionMediaNavigationRuntime::openPublishesTargetPlanAndCandidates()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    KiriView::DocumentSessionMediaNavigationOpenResult result;

    fixture.runtime.open(
        &fixture.receiver, mediaScope(currentUrl), KiriView::nextMediaNavigationOpenRequest(),
        [](const KiriView::DirectMediaScope &) { return true; },
        [&fixture, &result](KiriView::DocumentSessionMediaNavigationOpenResult loadResult) {
            ++fixture.completionCount;
            result = std::move(loadResult);
        });
    fixture.provider.deliverIgnoringCancellation(
        0, { mediaCandidate(currentUrl), mediaCandidate(nextUrl) });

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(result.succeeded);
    QCOMPARE(result.candidates.size(), std::size_t(2));
    QVERIFY(result.plan.targetUrl.has_value());
    QCOMPARE(*result.plan.targetUrl, nextUrl);
    QCOMPARE(result.plan.boundaryState.currentNumber, 1);
    QCOMPARE(result.plan.boundaryState.count, 2);
}

void TestDocumentSessionMediaNavigationRuntime::failedOpenPublishesUnknownResult()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));
    KiriView::DocumentSessionMediaNavigationOpenResult result;

    fixture.runtime.open(
        &fixture.receiver, mediaScope(currentUrl), KiriView::nextMediaNavigationOpenRequest(),
        [](const KiriView::DirectMediaScope &) { return true; },
        [&fixture, &result](KiriView::DocumentSessionMediaNavigationOpenResult loadResult) {
            ++fixture.completionCount;
            result = std::move(loadResult);
        });
    fixture.provider.deliverErrorIgnoringCancellation(0, QStringLiteral("missing media"));

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(!result.succeeded);
    QVERIFY(!result.plan.targetUrl.has_value());
    QCOMPARE(result.errorString, QStringLiteral("missing media"));
}

void TestDocumentSessionMediaNavigationRuntime::cancelRejectsLateCompletion()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.load(mediaScope(currentUrl));
    fixture.runtime.cancel();
    QVERIFY(fixture.provider.loadAt(0).canceled);
    fixture.provider.deliverIgnoringCancellation(0, { mediaCandidate(currentUrl) });

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaNavigationRuntime::cancelRejectsLateError()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.load(mediaScope(currentUrl));
    fixture.runtime.cancel();
    QVERIFY(fixture.provider.loadAt(0).canceled);
    fixture.provider.deliverErrorIgnoringCancellation(0, QStringLiteral("failed"));

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaNavigationRuntime::predicateRejectionDropsCurrentCompletion()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.acceptScope = false;
    fixture.load(mediaScope(currentUrl));
    fixture.provider.deliverIgnoringCancellation(0, { mediaCandidate(currentUrl) });

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaNavigationRuntime::acceptedScopeAllowsEquivalentCursorConfirmation()
{
    RuntimeFixture fixture;
    const QUrl currentUrl(QStringLiteral("file:///media/./01.mp4"));
    const QUrl confirmedUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.runtime.loadCandidates(
        &fixture.receiver, mediaScope(currentUrl),
        [confirmedUrl](const KiriView::DirectMediaScope &scope) {
            return KiriView::sameNormalizedUrl(scope.currentUrl, confirmedUrl)
                && scope.generation == 7;
        },
        [&fixture](KiriView::DocumentSessionMediaNavigationCandidatesResult loadResult) {
            ++fixture.completionCount;
            fixture.result = std::move(loadResult);
        });
    fixture.provider.deliverIgnoringCancellation(0, { mediaCandidate(confirmedUrl) });

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(fixture.result.succeeded);
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaNavigationRuntime)

#include "test_documentsessionmedianavigationruntime.moc"
