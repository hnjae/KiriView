// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediaopenwithruntime.h"

#include "async/imageiojob.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

class TestDocumentSessionMediaOpenWithRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyPlanDoesNotStartProviderAndReportsFailure();
    void startRunsProviderAndPublishesSuccess();
    void failedProviderCompletionForwardsTypedFailure();
    void canceledProviderCompletionForwardsCanceledResult();
    void cancelRejectsLateCompletion();
    void replacementStartRejectsStaleCompletion();
    void destructionCancelsAndRejectsLateCompletion();
};

namespace {
QUrl localUrl(const QString& path) { return QUrl::fromLocalFile(path); }

kiriview::MediaOpenWithPlan openWithPlanFor(const QUrl& targetUrl)
{
    return kiriview::MediaOpenWithPlan { kiriview::MediaOpenWithRequest { targetUrl } };
}

kiriview::KioOperationFailure openWithFailure(
    const QUrl& targetUrl, kiriview::MediaOpenWithResult result, const QString& errorString)
{
    kiriview::KioOperationFailure failure;
    failure.operationKind = kiriview::KioOperationKind::MediaOpenWith;
    failure.targetUrl = targetUrl;
    failure.canceled = result == kiriview::MediaOpenWithResult::Canceled;
    failure.userMessage = result == kiriview::MediaOpenWithResult::Failed ? errorString : QString();
    failure.diagnosticDetail = errorString;
    failure.retryable = result == kiriview::MediaOpenWithResult::Failed;
    return failure;
}

struct ManualMediaOpenWithOperation
{
    QObject* object = nullptr;
    kiriview::MediaOpenWithRequest request;
    kiriview::MediaOpenWithCallback callback;
    kiriview::ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualMediaOpenWithProvider
{
public:
    kiriview::MediaOpenWithProvider provider()
    {
        return [this](QObject* receiver, kiriview::MediaOpenWithRequest request,
                   kiriview::MediaOpenWithCallback callback) {
            auto operation = std::make_shared<ManualMediaOpenWithOperation>();
            operation->request = std::move(request);
            operation->callback = std::move(callback);
            operation->object = new QObject(receiver);

            std::weak_ptr<ManualMediaOpenWithOperation> weakOperation = operation;
            kiriview::ImageIoJob job(operation->object, [weakOperation](QObject* object) {
                if (std::shared_ptr<ManualMediaOpenWithOperation> operation
                    = weakOperation.lock()) {
                    operation->canceled = true;
                    operation->object = nullptr;
                }
                if (object != nullptr) {
                    object->deleteLater();
                }
            });
            operation->completion = job.completion();
            m_operations.push_back(operation);
            return job;
        };
    }

    std::size_t operationCount() const { return m_operations.size(); }

    ManualMediaOpenWithOperation& operationAt(std::size_t index) { return *m_operations.at(index); }

    void finishOperationAt(std::size_t index, kiriview::MediaOpenWithResult result,
        const QString& errorString = QString())
    {
        finishOperationAt(index, result,
            openWithFailure(m_operations.at(index)->request.targetUrl, result, errorString));
    }

    void finishOperationAt(std::size_t index, kiriview::MediaOpenWithResult result,
        kiriview::KioOperationFailure failure)
    {
        finishOperation(m_operations.at(index), result, std::move(failure));
    }

    void deliverOperationAtIgnoringCancellation(std::size_t index,
        kiriview::MediaOpenWithResult result, const QString& errorString = QString())
    {
        deliverOperationAtIgnoringCancellation(index, result,
            openWithFailure(m_operations.at(index)->request.targetUrl, result, errorString));
    }

    void deliverOperationAtIgnoringCancellation(std::size_t index,
        kiriview::MediaOpenWithResult result, const kiriview::KioOperationFailure& failure)
    {
        if (m_operations.at(index)->callback) {
            m_operations.at(index)->callback(result, failure);
        }
    }

private:
    static void finishOperation(const std::shared_ptr<ManualMediaOpenWithOperation>& operation,
        kiriview::MediaOpenWithResult result, kiriview::KioOperationFailure failure)
    {
        QObject* object = operation->object;
        operation->completion.claimAndRun([&]() {
            operation->object = nullptr;
            if (operation->callback) {
                operation->callback(result, failure);
            }
            if (object != nullptr) {
                object->deleteLater();
            }
        });
    }

    std::vector<std::shared_ptr<ManualMediaOpenWithOperation>> m_operations;
};

struct CompletionRecord
{
    kiriview::MediaOpenWithResult result = kiriview::MediaOpenWithResult::Succeeded;
    kiriview::KioOperationFailure failure;
};

struct RuntimeFixture
{
    QObject receiver;
    ManualMediaOpenWithProvider provider;
    kiriview::DocumentSessionMediaOpenWithRuntime runtime { provider.provider() };
    std::vector<CompletionRecord> completions;

    void open(const kiriview::MediaOpenWithPlan& plan)
    {
        runtime.open(&receiver, plan,
            [this](kiriview::MediaOpenWithResult result,
                const kiriview::KioOperationFailure& failure) {
                completions.push_back(CompletionRecord { result, failure });
            });
    }
};
}

void TestDocumentSessionMediaOpenWithRuntime::emptyPlanDoesNotStartProviderAndReportsFailure()
{
    RuntimeFixture fixture;

    fixture.open(kiriview::MediaOpenWithPlan {});

    QCOMPARE(fixture.provider.operationCount(), std::size_t(0));
    QVERIFY(!fixture.runtime.active());
    QCOMPARE(fixture.completions.size(), std::size_t(1));
    QCOMPARE(fixture.completions.at(0).result, kiriview::MediaOpenWithResult::Failed);
    QCOMPARE(
        fixture.completions.at(0).failure.operationKind, kiriview::KioOperationKind::MediaOpenWith);
    QVERIFY(fixture.completions.at(0).failure.targetUrl.isEmpty());
    QVERIFY(!fixture.completions.at(0).failure.rawErrorCode.has_value());
    QCOMPARE(fixture.completions.at(0).failure.userMessage, QString());
    QVERIFY(!fixture.completions.at(0).failure.diagnosticDetail.isEmpty());
}

void TestDocumentSessionMediaOpenWithRuntime::startRunsProviderAndPublishesSuccess()
{
    RuntimeFixture fixture;
    const QUrl targetUrl = localUrl(QStringLiteral("/media/01.png"));

    fixture.open(openWithPlanFor(targetUrl));

    QCOMPARE(fixture.provider.operationCount(), std::size_t(1));
    QCOMPARE(fixture.provider.operationAt(0).request.targetUrl, targetUrl);
    QVERIFY(fixture.runtime.active());

    fixture.provider.finishOperationAt(0, kiriview::MediaOpenWithResult::Succeeded);

    QVERIFY(!fixture.runtime.active());
    QCOMPARE(fixture.completions.size(), std::size_t(1));
    QCOMPARE(fixture.completions.at(0).result, kiriview::MediaOpenWithResult::Succeeded);
    QCOMPARE(fixture.completions.at(0).failure.userMessage, QString());
}

void TestDocumentSessionMediaOpenWithRuntime::failedProviderCompletionForwardsTypedFailure()
{
    RuntimeFixture fixture;
    const QUrl targetUrl = localUrl(QStringLiteral("/media/01.png"));

    fixture.open(openWithPlanFor(targetUrl));
    fixture.provider.finishOperationAt(
        0, kiriview::MediaOpenWithResult::Failed, QStringLiteral("launcher failed"));

    QCOMPARE(fixture.completions.size(), std::size_t(1));
    QCOMPARE(fixture.completions.at(0).result, kiriview::MediaOpenWithResult::Failed);
    QCOMPARE(
        fixture.completions.at(0).failure.operationKind, kiriview::KioOperationKind::MediaOpenWith);
    QCOMPARE(fixture.completions.at(0).failure.targetUrl, targetUrl);
    QCOMPARE(fixture.completions.at(0).failure.userMessage, QStringLiteral("launcher failed"));
    QCOMPARE(fixture.completions.at(0).failure.diagnosticDetail, QStringLiteral("launcher failed"));
    QVERIFY(fixture.completions.at(0).failure.retryable);
}

void TestDocumentSessionMediaOpenWithRuntime::canceledProviderCompletionForwardsCanceledResult()
{
    RuntimeFixture fixture;

    fixture.open(openWithPlanFor(localUrl(QStringLiteral("/media/01.png"))));
    fixture.provider.finishOperationAt(0, kiriview::MediaOpenWithResult::Canceled);

    QCOMPARE(fixture.completions.size(), std::size_t(1));
    QCOMPARE(fixture.completions.at(0).result, kiriview::MediaOpenWithResult::Canceled);
    QVERIFY(fixture.completions.at(0).failure.canceled);
    QCOMPARE(fixture.completions.at(0).failure.userMessage, QString());
}

void TestDocumentSessionMediaOpenWithRuntime::cancelRejectsLateCompletion()
{
    RuntimeFixture fixture;

    fixture.open(openWithPlanFor(localUrl(QStringLiteral("/media/01.png"))));
    fixture.runtime.cancel();

    QVERIFY(!fixture.runtime.active());
    QVERIFY(fixture.provider.operationAt(0).canceled);

    fixture.provider.deliverOperationAtIgnoringCancellation(
        0, kiriview::MediaOpenWithResult::Failed, QStringLiteral("late launcher failed"));

    QVERIFY(fixture.completions.empty());
}

void TestDocumentSessionMediaOpenWithRuntime::replacementStartRejectsStaleCompletion()
{
    RuntimeFixture fixture;

    fixture.open(openWithPlanFor(localUrl(QStringLiteral("/media/01.png"))));
    fixture.open(openWithPlanFor(localUrl(QStringLiteral("/media/02.png"))));

    QCOMPARE(fixture.provider.operationCount(), std::size_t(2));
    QVERIFY(fixture.provider.operationAt(0).canceled);
    QCOMPARE(fixture.provider.operationAt(1).request.targetUrl,
        localUrl(QStringLiteral("/media/02.png")));

    fixture.provider.deliverOperationAtIgnoringCancellation(
        0, kiriview::MediaOpenWithResult::Failed, QStringLiteral("stale launcher failed"));

    QVERIFY(fixture.completions.empty());

    fixture.provider.finishOperationAt(1, kiriview::MediaOpenWithResult::Succeeded);

    QCOMPARE(fixture.completions.size(), std::size_t(1));
    QCOMPARE(fixture.completions.at(0).result, kiriview::MediaOpenWithResult::Succeeded);
}

void TestDocumentSessionMediaOpenWithRuntime::destructionCancelsAndRejectsLateCompletion()
{
    ManualMediaOpenWithProvider provider;
    std::vector<CompletionRecord> completions;
    QObject receiver;

    {
        kiriview::DocumentSessionMediaOpenWithRuntime runtime(provider.provider());
        runtime.open(&receiver, openWithPlanFor(localUrl(QStringLiteral("/media/01.png"))),
            [&completions](kiriview::MediaOpenWithResult result,
                const kiriview::KioOperationFailure& failure) {
                completions.push_back(CompletionRecord { result, failure });
            });

        QCOMPARE(provider.operationCount(), std::size_t(1));
    }

    QVERIFY(provider.operationAt(0).canceled);

    provider.deliverOperationAtIgnoringCancellation(
        0, kiriview::MediaOpenWithResult::Failed, QStringLiteral("destroyed launcher failed"));

    QVERIFY(completions.empty());
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaOpenWithRuntime)

#include "test_documentsessionmediaopenwithruntime.moc"
