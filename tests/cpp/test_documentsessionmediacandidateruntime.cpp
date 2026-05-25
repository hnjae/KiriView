// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediacandidateruntime.h"

#include "image_async_test_support.h"
#include "location/imageurl.h"

#include <QObject>
#include <QTest>
#include <memory>
#include <utility>
#include <vector>

class TestDocumentSessionMediaCandidateRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void invalidScopeCompletesWithoutStartingProvider();
    void successfulLoadPublishesCandidates();
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
    KiriView::DocumentSessionMediaCandidateRuntime runtime { provider.provider() };
    int completionCount = 0;
    KiriView::MediaCandidateLoadResult result;
    bool acceptScope = true;

    void load(const KiriView::DocumentSessionMediaCandidateLoadScope &scope)
    {
        runtime.load(
            &receiver, scope,
            [this](
                const KiriView::DocumentSessionMediaCandidateLoadScope &) { return acceptScope; },
            [this](KiriView::MediaCandidateLoadResult loadResult) {
                ++completionCount;
                result = std::move(loadResult);
            });
    }
};

KiriView::DocumentSessionMediaCandidateLoadScope mediaScope(const QUrl &currentUrl)
{
    return KiriView::DocumentSessionMediaCandidateLoadScope {
        currentUrl,
        localUrl(QStringLiteral("/media/")),
        7,
    };
}
}

void TestDocumentSessionMediaCandidateRuntime::invalidScopeCompletesWithoutStartingProvider()
{
    RuntimeFixture fixture;

    fixture.load(KiriView::DocumentSessionMediaCandidateLoadScope {});

    QCOMPARE(fixture.provider.loadCount(), std::size_t(0));
    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(!fixture.result.succeeded);
    QVERIFY(fixture.result.candidates.empty());
}

void TestDocumentSessionMediaCandidateRuntime::successfulLoadPublishesCandidates()
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

void TestDocumentSessionMediaCandidateRuntime::cancelRejectsLateCompletion()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.load(mediaScope(currentUrl));
    fixture.runtime.cancel();
    QVERIFY(fixture.provider.loadAt(0).canceled);
    fixture.provider.deliverIgnoringCancellation(0, { mediaCandidate(currentUrl) });

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaCandidateRuntime::cancelRejectsLateError()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.load(mediaScope(currentUrl));
    fixture.runtime.cancel();
    QVERIFY(fixture.provider.loadAt(0).canceled);
    fixture.provider.deliverErrorIgnoringCancellation(0, QStringLiteral("failed"));

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaCandidateRuntime::predicateRejectionDropsCurrentCompletion()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.acceptScope = false;
    fixture.load(mediaScope(currentUrl));
    fixture.provider.deliverIgnoringCancellation(0, { mediaCandidate(currentUrl) });

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaCandidateRuntime::acceptedScopeAllowsEquivalentCursorConfirmation()
{
    RuntimeFixture fixture;
    const QUrl currentUrl(QStringLiteral("file:///media/./01.mp4"));
    const QUrl confirmedUrl = localUrl(QStringLiteral("/media/01.mp4"));

    fixture.runtime.load(
        &fixture.receiver, mediaScope(currentUrl),
        [confirmedUrl](const KiriView::DocumentSessionMediaCandidateLoadScope &scope) {
            return KiriView::sameNormalizedUrl(scope.currentUrl, confirmedUrl)
                && scope.cursorGeneration == 7;
        },
        [&fixture](KiriView::MediaCandidateLoadResult loadResult) {
            ++fixture.completionCount;
            fixture.result = std::move(loadResult);
        });
    fixture.provider.deliverIgnoringCancellation(0, { mediaCandidate(confirmedUrl) });

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(fixture.result.succeeded);
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaCandidateRuntime)

#include "test_documentsessionmediacandidateruntime.moc"
