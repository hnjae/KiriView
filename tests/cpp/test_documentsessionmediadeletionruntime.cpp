// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediadeletionruntime.h"

#include "image_async_test_support.h"
#include "navigation/directmedianavigationcandidateprovider.h"
#include "session/directmediacursor.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <utility>
#include <vector>

class TestDocumentSessionMediaDeletionRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyTargetDoesNotStartFileOperation();
    void startRunsFileOperationAndPublishesCompletionPlan();
    void directMediaStartLoadsCandidatesBeforeFileOperation();
    void directMediaCandidateLoadCancelRejectsLateCompletion();
    void cancelRejectsLateCompletion();
    void replacementStartRejectsStaleCompletion();
    void failedCompletionReportsFailureWithTypedFailure();
};

namespace {
QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl& url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

struct ManualDirectMediaNavigationCandidateLoad
{
    QObject* object = nullptr;
    QUrl parentUrl;
    kiriview::DirectMediaNavigationCandidatesCallback callback;
    kiriview::ErrorCallback errorCallback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualDirectMediaNavigationCandidateProvider
{
public:
    kiriview::DirectMediaNavigationCandidateProvider provider()
    {
        return kiriview::DirectMediaNavigationCandidateProvider {
            [this](QObject* receiver, QUrl parentUrl,
                kiriview::DirectMediaNavigationCandidatesCallback callback,
                kiriview::ErrorCallback errorCallback) {
                auto load = std::make_shared<ManualDirectMediaNavigationCandidateLoad>();
                load->parentUrl = std::move(parentUrl);
                load->callback = std::move(callback);
                load->errorCallback = std::move(errorCallback);

                kiriview::ImageIoJob job
                    = kiriview::TestSupport::Detail::startManualIoJob(receiver, load);
                m_loads.push_back(load);
                return job;
            },
        };
    }

    std::size_t loadCount() const { return m_loads.size(); }

    ManualDirectMediaNavigationCandidateLoad& loadAt(std::size_t index)
    {
        return *m_loads.at(index);
    }

    void deliverIgnoringCancellation(
        std::size_t index, std::vector<kiriview::DirectMediaNavigationCandidate> candidates)
    {
        ManualDirectMediaNavigationCandidateLoad& load = loadAt(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

private:
    std::vector<std::shared_ptr<ManualDirectMediaNavigationCandidateLoad>> m_loads;
};

kiriview::DirectMediaScope directMediaScope(const QUrl& currentUrl)
{
    return kiriview::DirectMediaScope {
        currentUrl,
        localUrl(QStringLiteral("/media/")),
        7,
    };
}

template <typename Operation>
const Operation* operationAt(
    const kiriview::DocumentSessionMediaDeletionCompletionPlan& plan, std::size_t index)
{
    if (index >= plan.routePlan.mutations.size()) {
        return nullptr;
    }

    return std::get_if<Operation>(&plan.routePlan.mutations.at(index));
}

struct RuntimeFixture
{
    QObject receiver;
    kiriview::TestSupport::ManualFileDeletionProvider fileDeletionProvider;
    ManualDirectMediaNavigationCandidateProvider candidateProvider;
    kiriview::DocumentSessionMediaDeletionRuntime runtime {
        kiriview::TestSupport::fileDeletionProviderFor(fileDeletionProvider),
        candidateProvider.provider(),
    };
    int completionCount = 0;
    kiriview::DocumentSessionMediaDeletionCompletion completion;
    bool acceptScope = true;

    kiriview::DocumentSessionMediaDeletionStartPlan start(kiriview::FileDeletionMode mode,
        std::vector<kiriview::DirectMediaNavigationCandidate> candidates, const QUrl& currentUrl,
        kiriview::DocumentSessionKind kind = kiriview::DocumentSessionKind::Video)
    {
        return runtime.start(&receiver, mode, std::move(candidates), currentUrl, kind,
            [this](kiriview::DocumentSessionMediaDeletionCompletion deletionCompletion) {
                ++completionCount;
                completion = std::move(deletionCompletion);
            });
    }

    bool startDirectMedia(kiriview::FileDeletionMode mode, const kiriview::DirectMediaScope& scope,
        kiriview::DocumentSessionKind kind = kiriview::DocumentSessionKind::Video)
    {
        return runtime.startForDirectMedia(
            &receiver, mode, scope,
            [this](const kiriview::DirectMediaScope&) { return acceptScope; }, kind,
            [this](kiriview::DocumentSessionMediaDeletionCompletion deletionCompletion) {
                ++completionCount;
                completion = std::move(deletionCompletion);
            });
    }
};
}

void TestDocumentSessionMediaDeletionRuntime::emptyTargetDoesNotStartFileOperation()
{
    RuntimeFixture fixture;

    const kiriview::DocumentSessionMediaDeletionStartPlan plan = fixture.start(
        kiriview::FileDeletionMode::MoveToTrash, {}, QUrl(), kiriview::DocumentSessionKind::Video);

    QVERIFY(!plan.shouldStartDeletion);
    QCOMPARE(fixture.fileDeletionProvider.operationCount(), std::size_t(0));
    QVERIFY(!fixture.runtime.active());
    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaDeletionRuntime::startRunsFileOperationAndPublishesCompletionPlan()
{
    RuntimeFixture fixture;
    const QUrl previousUrl = localUrl(QStringLiteral("/media/01.jpg"));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/03.png"));

    const kiriview::DocumentSessionMediaDeletionStartPlan startPlan = fixture.start(
        kiriview::FileDeletionMode::DeletePermanently,
        { directMediaNavigationCandidate(previousUrl), directMediaNavigationCandidate(currentUrl),
            directMediaNavigationCandidate(nextUrl) },
        currentUrl);

    QVERIFY(startPlan.shouldStartDeletion);
    QCOMPARE(fixture.fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fixture.fileDeletionProvider.backOperation().request.targetUrl, currentUrl);
    QCOMPARE(fixture.fileDeletionProvider.backOperation().request.mode,
        kiriview::FileDeletionMode::DeletePermanently);
    QVERIFY(fixture.runtime.active());
    QCOMPARE(startPlan.fallbackPlan.preferredFallbackUrl.value(), nextUrl);

    fixture.fileDeletionProvider.finishBackOperation(kiriview::FileDeletionResult::Succeeded);

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(!fixture.runtime.active());
    QVERIFY(fixture.completion.plan.hasRoutePlan());
    QCOMPARE(
        fixture.completion.plan.routePlan.kind, kiriview::DocumentSessionRouteKind::DirectImage);
    QCOMPARE(fixture.completion.plan.routePlan.sourceUrl, nextUrl);
    QVERIFY(
        operationAt<kiriview::LeaveVideoModeRouteOperation>(fixture.completion.plan, 0) != nullptr);
    QCOMPARE(fixture.completion.failure.userMessage, QString());
}

void TestDocumentSessionMediaDeletionRuntime::directMediaStartLoadsCandidatesBeforeFileOperation()
{
    RuntimeFixture fixture;
    const QUrl previousUrl = localUrl(QStringLiteral("/media/01.jpg"));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/03.png"));

    const bool started = fixture.startDirectMedia(
        kiriview::FileDeletionMode::MoveToTrash, directMediaScope(currentUrl));

    QVERIFY(started);
    QCOMPARE(fixture.candidateProvider.loadCount(), std::size_t(1));
    QCOMPARE(fixture.candidateProvider.loadAt(0).parentUrl, localUrl(QStringLiteral("/media/")));
    QCOMPARE(fixture.fileDeletionProvider.operationCount(), std::size_t(0));

    fixture.candidateProvider.deliverIgnoringCancellation(0,
        { directMediaNavigationCandidate(previousUrl), directMediaNavigationCandidate(currentUrl),
            directMediaNavigationCandidate(nextUrl) });

    QCOMPARE(fixture.fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fixture.fileDeletionProvider.backOperation().request.targetUrl, currentUrl);

    fixture.fileDeletionProvider.finishBackOperation(kiriview::FileDeletionResult::Succeeded);

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(fixture.completion.plan.hasRoutePlan());
    QCOMPARE(fixture.completion.plan.routePlan.sourceUrl, nextUrl);
}

void TestDocumentSessionMediaDeletionRuntime::directMediaCandidateLoadCancelRejectsLateCompletion()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.mp4"));

    QVERIFY(fixture.startDirectMedia(
        kiriview::FileDeletionMode::MoveToTrash, directMediaScope(currentUrl)));
    fixture.runtime.cancel();

    QVERIFY(fixture.candidateProvider.loadAt(0).canceled);
    fixture.candidateProvider.deliverIgnoringCancellation(
        0, { directMediaNavigationCandidate(currentUrl) });

    QCOMPARE(fixture.fileDeletionProvider.operationCount(), std::size_t(0));
    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaDeletionRuntime::cancelRejectsLateCompletion()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.mp4"));

    fixture.start(kiriview::FileDeletionMode::MoveToTrash,
        { directMediaNavigationCandidate(currentUrl) }, currentUrl);
    fixture.runtime.cancel();
    QVERIFY(fixture.fileDeletionProvider.backOperation().canceled);
    QVERIFY(!fixture.runtime.active());

    fixture.fileDeletionProvider.deliverOperationAtIgnoringCancellation(
        0, kiriview::FileDeletionResult::Succeeded);

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaDeletionRuntime::replacementStartRejectsStaleCompletion()
{
    RuntimeFixture fixture;
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl firstFallbackUrl = localUrl(QStringLiteral("/media/02.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/03.mp4"));
    const QUrl secondFallbackUrl = localUrl(QStringLiteral("/media/04.png"));

    fixture.start(kiriview::FileDeletionMode::MoveToTrash,
        { directMediaNavigationCandidate(firstUrl),
            directMediaNavigationCandidate(firstFallbackUrl) },
        firstUrl);
    fixture.start(kiriview::FileDeletionMode::MoveToTrash,
        { directMediaNavigationCandidate(secondUrl),
            directMediaNavigationCandidate(secondFallbackUrl) },
        secondUrl);

    QCOMPARE(fixture.fileDeletionProvider.operationCount(), std::size_t(2));
    QVERIFY(fixture.fileDeletionProvider.operationAt(0).canceled);

    fixture.fileDeletionProvider.deliverOperationAtIgnoringCancellation(
        0, kiriview::FileDeletionResult::Succeeded);
    QCOMPARE(fixture.completionCount, 0);

    fixture.fileDeletionProvider.finishBackOperation(kiriview::FileDeletionResult::Succeeded);
    QCOMPARE(fixture.completionCount, 1);
    QCOMPARE(fixture.completion.plan.routePlan.sourceUrl, secondFallbackUrl);
}

void TestDocumentSessionMediaDeletionRuntime::failedCompletionReportsFailureWithTypedFailure()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.mp4"));

    fixture.start(kiriview::FileDeletionMode::MoveToTrash,
        { directMediaNavigationCandidate(currentUrl) }, currentUrl,
        kiriview::DocumentSessionKind::Image);
    fixture.fileDeletionProvider.finishBackOperation(
        kiriview::FileDeletionResult::Failed, QStringLiteral("delete failed"));

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(!fixture.completion.plan.hasRoutePlan());
    QVERIFY(fixture.completion.plan.reportFailure);
    QCOMPARE(fixture.completion.failure.operationKind, kiriview::KioOperationKind::FileDeletion);
    QCOMPARE(fixture.completion.failure.targetUrl, currentUrl);
    QCOMPARE(fixture.completion.failure.userMessage, QStringLiteral("delete failed"));
    QCOMPARE(fixture.completion.failure.diagnosticDetail, QStringLiteral("delete failed"));
    QVERIFY(fixture.completion.failure.retryable);
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaDeletionRuntime)

#include "test_documentsessionmediadeletionruntime.moc"
