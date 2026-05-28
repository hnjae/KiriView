// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmedianavigationruntime.h"

#include "image_async_test_support.h"
#include "location/imageurl.h"

#include <QObject>
#include <QTest>
#include <memory>
#include <utility>
#include <vector>

class TestDocumentSessionDirectMediaNavigationRuntime : public QObject
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

KiriView::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return KiriView::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

struct ManualDirectMediaNavigationCandidateLoad {
    QObject *object = nullptr;
    QUrl parentUrl;
    KiriView::DirectMediaNavigationCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
    KiriView::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualDirectMediaNavigationCandidateProvider
{
public:
    KiriView::DirectMediaNavigationCandidateProvider provider()
    {
        return KiriView::DirectMediaNavigationCandidateProvider {
            [this](QObject *receiver, QUrl parentUrl,
                KiriView::DirectMediaNavigationCandidatesCallback callback,
                KiriView::ErrorCallback errorCallback) {
                auto load = std::make_shared<ManualDirectMediaNavigationCandidateLoad>();
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

    ManualDirectMediaNavigationCandidateLoad &loadAt(std::size_t index)
    {
        return *m_loads.at(index);
    }

    void deliverIgnoringCancellation(
        std::size_t index, std::vector<KiriView::DirectMediaNavigationCandidate> candidates)
    {
        ManualDirectMediaNavigationCandidateLoad &load = loadAt(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

    void deliverErrorIgnoringCancellation(std::size_t index, const QString &errorString)
    {
        ManualDirectMediaNavigationCandidateLoad &load = loadAt(index);
        if (load.errorCallback) {
            load.errorCallback(errorString);
        }
    }

private:
    std::vector<std::shared_ptr<ManualDirectMediaNavigationCandidateLoad>> m_loads;
};

struct RuntimeFixture {
    QObject receiver;
    ManualDirectMediaNavigationCandidateProvider provider;
    KiriView::DocumentSessionDirectMediaNavigationRuntime runtime { provider.provider() };
    int completionCount = 0;
    KiriView::DocumentSessionDirectMediaNavigationCandidatesResult result;
    bool acceptScope = true;

    void load(const KiriView::DirectMediaScope &scope)
    {
        runtime.loadCandidates(
            &receiver, scope, [this](const KiriView::DirectMediaScope &) { return acceptScope; },
            [this](KiriView::DocumentSessionDirectMediaNavigationCandidatesResult loadResult) {
                ++completionCount;
                result = std::move(loadResult);
            });
    }
};

KiriView::DirectMediaScope directMediaScope(const QUrl &currentUrl)
{
    return KiriView::DirectMediaScope {
        currentUrl,
        localUrl(QStringLiteral("/media/")),
        7,
    };
}
}

void TestDocumentSessionDirectMediaNavigationRuntime::invalidScopeCompletesWithoutStartingProvider()
{
    RuntimeFixture fixture;

    fixture.load(KiriView::DirectMediaScope {});

    QCOMPARE(fixture.provider.loadCount(), std::size_t(0));
    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(!fixture.result.succeeded);
    QVERIFY(fixture.result.candidates.empty());
}

void TestDocumentSessionDirectMediaNavigationRuntime::successfulLoadPublishesCandidates()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));

    fixture.load(directMediaScope(currentUrl));
    fixture.provider.deliverIgnoringCancellation(
        0, { directMediaNavigationCandidate(currentUrl), directMediaNavigationCandidate(nextUrl) });

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(fixture.result.succeeded);
    QCOMPARE(fixture.result.candidates.size(), std::size_t(2));
    QCOMPARE(fixture.result.candidates.at(1).url, nextUrl);
}

void TestDocumentSessionDirectMediaNavigationRuntime::refreshPublishesBoundaryPlanAndCandidates()
{
    RuntimeFixture fixture;
    const QUrl previousUrl = localUrl(QStringLiteral("/media/00.mp4"));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    KiriView::DocumentSessionDirectMediaNavigationRefreshResult result;

    fixture.runtime.refresh(
        &fixture.receiver, directMediaScope(currentUrl),
        [](const KiriView::DirectMediaScope &) { return true; },
        [&fixture, &result](
            KiriView::DocumentSessionDirectMediaNavigationRefreshResult loadResult) {
            ++fixture.completionCount;
            result = std::move(loadResult);
        });
    fixture.provider.deliverIgnoringCancellation(0,
        { directMediaNavigationCandidate(previousUrl), directMediaNavigationCandidate(currentUrl),
            directMediaNavigationCandidate(nextUrl) });

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(result.succeeded);
    QCOMPARE(result.candidates.size(), std::size_t(3));
    QCOMPARE(result.boundaryState.currentNumber, 2);
    QCOMPARE(result.boundaryState.count, 3);
    QVERIFY(result.boundaryState.canOpenPrevious);
    QVERIFY(result.boundaryState.canOpenNext);
}

void TestDocumentSessionDirectMediaNavigationRuntime::openPublishesTargetPlanAndCandidates()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    KiriView::DocumentSessionDirectMediaNavigationOpenResult result;

    fixture.runtime.open(
        &fixture.receiver, directMediaScope(currentUrl),
        KiriView::nextDirectMediaNavigationOpenRequest(),
        [](const KiriView::DirectMediaScope &) { return true; },
        [&fixture, &result](KiriView::DocumentSessionDirectMediaNavigationOpenResult loadResult) {
            ++fixture.completionCount;
            result = std::move(loadResult);
        });
    fixture.provider.deliverIgnoringCancellation(
        0, { directMediaNavigationCandidate(currentUrl), directMediaNavigationCandidate(nextUrl) });

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(result.succeeded);
    QCOMPARE(result.candidates.size(), std::size_t(2));
    QVERIFY(result.plan.targetUrl.has_value());
    QCOMPARE(*result.plan.targetUrl, nextUrl);
    QCOMPARE(result.plan.boundaryState.currentNumber, 1);
    QCOMPARE(result.plan.boundaryState.count, 2);
}

void TestDocumentSessionDirectMediaNavigationRuntime::failedOpenPublishesUnknownResult()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));
    KiriView::DocumentSessionDirectMediaNavigationOpenResult result;

    fixture.runtime.open(
        &fixture.receiver, directMediaScope(currentUrl),
        KiriView::nextDirectMediaNavigationOpenRequest(),
        [](const KiriView::DirectMediaScope &) { return true; },
        [&fixture, &result](KiriView::DocumentSessionDirectMediaNavigationOpenResult loadResult) {
            ++fixture.completionCount;
            result = std::move(loadResult);
        });
    fixture.provider.deliverErrorIgnoringCancellation(0, QStringLiteral("missing media"));

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(!result.succeeded);
    QVERIFY(!result.plan.targetUrl.has_value());
    QCOMPARE(result.errorString, QStringLiteral("missing media"));
}

void TestDocumentSessionDirectMediaNavigationRuntime::cancelRejectsLateCompletion()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.load(directMediaScope(currentUrl));
    fixture.runtime.cancel();
    QVERIFY(fixture.provider.loadAt(0).canceled);
    fixture.provider.deliverIgnoringCancellation(0, { directMediaNavigationCandidate(currentUrl) });

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionDirectMediaNavigationRuntime::cancelRejectsLateError()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.load(directMediaScope(currentUrl));
    fixture.runtime.cancel();
    QVERIFY(fixture.provider.loadAt(0).canceled);
    fixture.provider.deliverErrorIgnoringCancellation(0, QStringLiteral("failed"));

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionDirectMediaNavigationRuntime::predicateRejectionDropsCurrentCompletion()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.acceptScope = false;
    fixture.load(directMediaScope(currentUrl));
    fixture.provider.deliverIgnoringCancellation(0, { directMediaNavigationCandidate(currentUrl) });

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionDirectMediaNavigationRuntime::
    acceptedScopeAllowsEquivalentCursorConfirmation()
{
    RuntimeFixture fixture;
    const QUrl currentUrl(QStringLiteral("file:///media/./01.mp4"));
    const QUrl confirmedUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.runtime.loadCandidates(
        &fixture.receiver, directMediaScope(currentUrl),
        [confirmedUrl](const KiriView::DirectMediaScope &scope) {
            return KiriView::sameNormalizedUrl(scope.currentUrl, confirmedUrl)
                && scope.generation == 7;
        },
        [&fixture](KiriView::DocumentSessionDirectMediaNavigationCandidatesResult loadResult) {
            ++fixture.completionCount;
            fixture.result = std::move(loadResult);
        });
    fixture.provider.deliverIgnoringCancellation(
        0, { directMediaNavigationCandidate(confirmedUrl) });

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(fixture.result.succeeded);
}

QTEST_GUILESS_MAIN(TestDocumentSessionDirectMediaNavigationRuntime)

#include "test_documentsessiondirectmedianavigationruntime.moc"
