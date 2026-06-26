// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediadeletioncompletionruntime.h"

#include "localization/imageerrortext.h"

#include <QObject>
#include <QString>
#include <QTest>
#include <QUrl>
#include <utility>
#include <vector>

class TestDocumentSessionMediaDeletionCompletionRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void failureClearsProgressPublishesErrorAndSkipsRoute();
    void successClearsProgressPublishesThenExecutesRoute();
};

namespace {
struct RuntimeProbe
{
    std::vector<QString> operations;
    std::vector<bool> progressValues;
    std::vector<QString> errors;
    std::vector<kiriview::DocumentSessionRoutePlan> routePlans;

    kiriview::DocumentSessionMediaDeletionCompletionRuntime runtime {
        kiriview::DocumentSessionMediaDeletionCompletionRuntimePorts {
            [this](bool inProgress) {
                operations.push_back(QStringLiteral("progress"));
                progressValues.push_back(inProgress);
            },
            [this](const QString& errorString) {
                operations.push_back(QStringLiteral("error"));
                errors.push_back(errorString);
            },
            [this]() { operations.push_back(QStringLiteral("publish")); },
            [this](const kiriview::DocumentSessionRoutePlan& plan) {
                operations.push_back(QStringLiteral("route"));
                routePlans.push_back(plan);
            },
        },
    };
};

kiriview::DocumentSessionMediaDeletionCompletion failedCompletion(QString userMessage = {})
{
    kiriview::DocumentSessionMediaDeletionCompletion completion;
    completion.plan.reportFailure = true;
    completion.failure.userMessage = std::move(userMessage);
    return completion;
}

kiriview::DocumentSessionMediaDeletionCompletion routedCompletion(const QUrl& targetUrl)
{
    kiriview::DocumentSessionMediaDeletionCompletion completion;
    completion.plan.routePlan.kind = kiriview::DocumentSessionRouteKind::DirectImage;
    completion.plan.routePlan.sourceUrl = targetUrl;
    completion.plan.routePlan.publishPublicProjection = true;
    return completion;
}
}

void TestDocumentSessionMediaDeletionCompletionRuntime::
    failureClearsProgressPublishesErrorAndSkipsRoute()
{
    RuntimeProbe probe;

    probe.runtime.apply(failedCompletion());

    const std::vector<QString> expectedOperations { QStringLiteral("progress"),
        QStringLiteral("error"), QStringLiteral("publish") };
    QCOMPARE(probe.operations, expectedOperations);
    QCOMPARE(probe.progressValues, std::vector<bool> { false });
    QCOMPARE(probe.errors,
        std::vector<QString> { kiriview::imageErrorText(kiriview::ImageErrorTextId::DeleteFile) });
    QVERIFY(probe.routePlans.empty());
}

void TestDocumentSessionMediaDeletionCompletionRuntime::
    successClearsProgressPublishesThenExecutesRoute()
{
    RuntimeProbe probe;
    const QUrl targetUrl = QUrl::fromLocalFile(QStringLiteral("/media/next.png"));

    probe.runtime.apply(routedCompletion(targetUrl));

    const std::vector<QString> expectedOperations { QStringLiteral("progress"),
        QStringLiteral("publish"), QStringLiteral("route") };
    QCOMPARE(probe.operations, expectedOperations);
    QCOMPARE(probe.progressValues, std::vector<bool> { false });
    QVERIFY(probe.errors.empty());
    QCOMPARE(probe.routePlans.size(), std::size_t(1));
    QCOMPARE(probe.routePlans.front().sourceUrl, targetUrl);
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaDeletionCompletionRuntime)

#include "test_documentsessionmediadeletioncompletionruntime.moc"
