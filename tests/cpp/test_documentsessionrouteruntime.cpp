// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionrouteruntime.h"

#include <QObject>
#include <QTest>
#include <QUrl>

#include <vector>

class TestDocumentSessionRouteRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void routeSourceUrlPlansAndExecutesFromCurrentKind();
    void routeMediaUrlPlansAndExecutesFromCurrentKind();
    void executionRunsMutationPublicationFollowUpAndCompletionInOrder();
    void executionPublishesBeforeTypedFollowUps();
    void clearedNavigationRepublishesBeforePredecodeClear();
    void activeNavigationRefreshesWithoutScopeChange();
};

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }
}

void TestDocumentSessionRouteRuntime::executionRunsMutationPublicationFollowUpAndCompletionInOrder()
{
    std::vector<QString> events;
    kiriview::DocumentSessionRouteRuntimePorts ports;
    ports.cancelMediaOpenWith
        = [&events]() { events.push_back(QStringLiteral("cancel-open-with")); };
    ports.clearSessionErrorString
        = [&events]() { events.push_back(QStringLiteral("clear-error")); };
    ports.clearDirectMediaCursor = [&events]() {
        events.push_back(QStringLiteral("clear-cursor"));
        return true;
    };
    ports.executeWithRoutingSuppressed = [&events](const std::function<void()> &mutation) {
        events.push_back(QStringLiteral("suppress-begin"));
        mutation();
        events.push_back(QStringLiteral("suppress-end"));
    };
    ports.enterImageDocument = [&events](const QUrl &url) {
        events.push_back(QStringLiteral("enter-image:%1").arg(url.toString()));
    };
    ports.useOriginalSourceIdentity = [&events](const QUrl &url) {
        events.push_back(QStringLiteral("identity:%1").arg(url.toString()));
    };
    ports.recomputePublicProjection = [&events]() { events.push_back(QStringLiteral("publish")); };
    ports.directMediaNavigationActive = []() { return false; };
    ports.refreshDirectMediaNavigation
        = [&events]() { events.push_back(QStringLiteral("refresh-navigation")); };
    ports.clearMediaPredecode
        = [&events]() { events.push_back(QStringLiteral("clear-predecode")); };
    ports.routeCompleted = [&events]() { events.push_back(QStringLiteral("complete")); };

    kiriview::DocumentSessionRouteRuntime runtime(std::move(ports));
    const QUrl imageUrl = localUrl(QStringLiteral("/tmp/page.png"));
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::DirectImage;
    plan.sourceUrl = imageUrl;
    plan.mutations = {
        kiriview::DocumentSessionRouteMutation {
            kiriview::ClearSessionErrorStringRouteOperation {} },
        kiriview::DocumentSessionRouteMutation {
            kiriview::ClearDirectMediaCursorRouteOperation {} },
        kiriview::DocumentSessionRouteMutation {
            kiriview::EnterImageDocumentRouteOperation { imageUrl } },
        kiriview::DocumentSessionRouteMutation {
            kiriview::UseOriginalSourceIdentityRouteOperation { imageUrl } },
    };
    plan.publishPublicProjection = true;
    plan.followUpEffects = {
        kiriview::DocumentSessionRouteFollowUpEffect {
            kiriview::RefreshDirectMediaNavigationAfterRoutingRouteEffect {} },
        kiriview::DocumentSessionRouteFollowUpEffect {
            kiriview::ClearMediaPredecodeRouteEffect {} },
    };

    runtime.execute(plan);

    const std::vector<QString> expected {
        QStringLiteral("cancel-open-with"),
        QStringLiteral("clear-error"),
        QStringLiteral("clear-cursor"),
        QStringLiteral("suppress-begin"),
        QStringLiteral("enter-image:%1").arg(imageUrl.toString()),
        QStringLiteral("suppress-end"),
        QStringLiteral("identity:%1").arg(imageUrl.toString()),
        QStringLiteral("publish"),
        QStringLiteral("refresh-navigation"),
        QStringLiteral("clear-predecode"),
        QStringLiteral("complete"),
    };
    QCOMPARE(events, expected);
}

void TestDocumentSessionRouteRuntime::routeSourceUrlPlansAndExecutesFromCurrentKind()
{
    std::vector<QString> events;
    kiriview::DocumentSessionRouteRuntimePorts ports;
    ports.cancelMediaOpenWith
        = [&events]() { events.push_back(QStringLiteral("cancel-open-with")); };
    ports.clearSessionErrorString
        = [&events]() { events.push_back(QStringLiteral("clear-error")); };
    ports.cancelDirectMediaNavigation
        = [&events]() { events.push_back(QStringLiteral("cancel-navigation")); };
    ports.cancelMediaDeletion
        = [&events]() { events.push_back(QStringLiteral("cancel-deletion")); };
    ports.clearDirectMediaNavigation
        = [&events]() { events.push_back(QStringLiteral("clear-navigation")); };
    ports.clearDirectMediaCursor = [&events]() {
        events.push_back(QStringLiteral("clear-cursor"));
        return true;
    };
    ports.requestDirectImageCursor = [&events](const QUrl &url) {
        events.push_back(QStringLiteral("request-image-cursor:%1").arg(url.toString()));
        return true;
    };
    ports.executeWithRoutingSuppressed = [&events](const std::function<void()> &mutation) {
        events.push_back(QStringLiteral("suppress-begin"));
        mutation();
        events.push_back(QStringLiteral("suppress-end"));
    };
    ports.leaveVideoMode = [&events]() { events.push_back(QStringLiteral("leave-video")); };
    ports.enterImageDocument = [&events](const QUrl &url) {
        events.push_back(QStringLiteral("enter-image:%1").arg(url.toString()));
    };
    ports.syncDirectImageCursorFromDocument = [&events]() {
        events.push_back(QStringLiteral("sync-image-cursor"));
        return true;
    };
    ports.useImageDocumentSourceIdentity
        = [&events]() { events.push_back(QStringLiteral("image-document-identity")); };
    ports.recomputePublicProjection = [&events]() { events.push_back(QStringLiteral("publish")); };
    ports.directMediaNavigationActive = []() { return false; };
    ports.refreshDirectMediaNavigation
        = [&events]() { events.push_back(QStringLiteral("refresh-navigation")); };
    ports.routeCompleted = [&events]() { events.push_back(QStringLiteral("complete")); };

    kiriview::DocumentSessionRouteRuntime runtime(std::move(ports));
    const QUrl imageUrl = localUrl(QStringLiteral("/tmp/page.png"));

    runtime.routeSourceUrl(imageUrl, kiriview::DocumentSessionKind::Empty);

    const std::vector<QString> expected {
        QStringLiteral("cancel-open-with"),
        QStringLiteral("clear-error"),
        QStringLiteral("cancel-navigation"),
        QStringLiteral("cancel-deletion"),
        QStringLiteral("clear-navigation"),
        QStringLiteral("clear-cursor"),
        QStringLiteral("request-image-cursor:%1").arg(imageUrl.toString()),
        QStringLiteral("suppress-begin"),
        QStringLiteral("leave-video"),
        QStringLiteral("suppress-end"),
        QStringLiteral("suppress-begin"),
        QStringLiteral("enter-image:%1").arg(imageUrl.toString()),
        QStringLiteral("suppress-end"),
        QStringLiteral("sync-image-cursor"),
        QStringLiteral("image-document-identity"),
        QStringLiteral("publish"),
        QStringLiteral("refresh-navigation"),
        QStringLiteral("complete"),
    };
    QCOMPARE(events, expected);
}

void TestDocumentSessionRouteRuntime::routeMediaUrlPlansAndExecutesFromCurrentKind()
{
    std::vector<QString> events;
    kiriview::DocumentSessionRouteRuntimePorts ports;
    ports.cancelMediaOpenWith
        = [&events]() { events.push_back(QStringLiteral("cancel-open-with")); };
    ports.setDirectVideoCursor = [&events](const QUrl &url) {
        events.push_back(QStringLiteral("video-cursor:%1").arg(url.toString()));
        return true;
    };
    ports.executeWithRoutingSuppressed = [&events](const std::function<void()> &mutation) {
        events.push_back(QStringLiteral("suppress-begin"));
        mutation();
        events.push_back(QStringLiteral("suppress-end"));
    };
    ports.enterVideoDocument = [&events](const QUrl &url) {
        events.push_back(QStringLiteral("enter-video:%1").arg(url.toString()));
    };
    ports.clearImageDocument = [&events]() { events.push_back(QStringLiteral("clear-image")); };
    ports.useOriginalSourceIdentity = [&events](const QUrl &url) {
        events.push_back(QStringLiteral("identity:%1").arg(url.toString()));
    };
    ports.recomputePublicProjection = [&events]() { events.push_back(QStringLiteral("publish")); };
    ports.directMediaNavigationActive = []() { return false; };
    ports.refreshDirectMediaNavigation
        = [&events]() { events.push_back(QStringLiteral("refresh-navigation")); };
    ports.routeCompleted = [&events]() { events.push_back(QStringLiteral("complete")); };

    kiriview::DocumentSessionRouteRuntime runtime(std::move(ports));
    const QUrl videoUrl = localUrl(QStringLiteral("/tmp/movie.mp4"));

    runtime.routeMediaUrl(videoUrl, kiriview::DocumentSessionKind::Image);

    const std::vector<QString> expected {
        QStringLiteral("cancel-open-with"),
        QStringLiteral("video-cursor:%1").arg(videoUrl.toString()),
        QStringLiteral("suppress-begin"),
        QStringLiteral("clear-image"),
        QStringLiteral("suppress-end"),
        QStringLiteral("suppress-begin"),
        QStringLiteral("enter-video:%1").arg(videoUrl.toString()),
        QStringLiteral("suppress-end"),
        QStringLiteral("identity:%1").arg(videoUrl.toString()),
        QStringLiteral("publish"),
        QStringLiteral("refresh-navigation"),
        QStringLiteral("complete"),
    };
    QCOMPARE(events, expected);
}

void TestDocumentSessionRouteRuntime::executionPublishesBeforeTypedFollowUps()
{
    std::vector<QString> events;
    kiriview::DocumentSessionRouteRuntimePorts ports;
    ports.cancelMediaOpenWith
        = [&events]() { events.push_back(QStringLiteral("cancel-open-with")); };
    ports.clearSessionErrorString
        = [&events]() { events.push_back(QStringLiteral("clear-error")); };
    ports.clearDirectMediaCursor = [&events]() {
        events.push_back(QStringLiteral("clear-cursor"));
        return true;
    };
    ports.executeWithRoutingSuppressed = [&events](const std::function<void()> &mutation) {
        events.push_back(QStringLiteral("suppress-begin"));
        mutation();
        events.push_back(QStringLiteral("suppress-end"));
    };
    ports.enterImageDocument = [&events](const QUrl &url) {
        events.push_back(QStringLiteral("enter-image:%1").arg(url.toString()));
    };
    ports.useOriginalSourceIdentity = [&events](const QUrl &url) {
        events.push_back(QStringLiteral("identity:%1").arg(url.toString()));
    };
    ports.recomputePublicProjection = [&events]() { events.push_back(QStringLiteral("publish")); };
    ports.directMediaNavigationActive = []() { return false; };
    ports.refreshDirectMediaNavigation
        = [&events]() { events.push_back(QStringLiteral("refresh-navigation")); };
    ports.clearMediaPredecode
        = [&events]() { events.push_back(QStringLiteral("clear-predecode")); };
    ports.routeCompleted = [&events]() { events.push_back(QStringLiteral("complete")); };

    kiriview::DocumentSessionRouteRuntime runtime(std::move(ports));
    const QUrl imageUrl = localUrl(QStringLiteral("/tmp/page.png"));
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::DirectImage;
    plan.sourceUrl = imageUrl;
    plan.mutations = {
        kiriview::DocumentSessionRouteMutation {
            kiriview::ClearSessionErrorStringRouteOperation {} },
        kiriview::DocumentSessionRouteMutation {
            kiriview::ClearDirectMediaCursorRouteOperation {} },
        kiriview::DocumentSessionRouteMutation {
            kiriview::EnterImageDocumentRouteOperation { imageUrl } },
        kiriview::DocumentSessionRouteMutation {
            kiriview::UseOriginalSourceIdentityRouteOperation { imageUrl } },
    };
    plan.publishPublicProjection = true;
    plan.followUpEffects = {
        kiriview::DocumentSessionRouteFollowUpEffect {
            kiriview::RefreshDirectMediaNavigationAfterRoutingRouteEffect {} },
        kiriview::DocumentSessionRouteFollowUpEffect {
            kiriview::ClearMediaPredecodeRouteEffect {} },
    };

    runtime.execute(plan);

    const std::vector<QString> expected {
        QStringLiteral("cancel-open-with"),
        QStringLiteral("clear-error"),
        QStringLiteral("clear-cursor"),
        QStringLiteral("suppress-begin"),
        QStringLiteral("enter-image:%1").arg(imageUrl.toString()),
        QStringLiteral("suppress-end"),
        QStringLiteral("identity:%1").arg(imageUrl.toString()),
        QStringLiteral("publish"),
        QStringLiteral("refresh-navigation"),
        QStringLiteral("clear-predecode"),
        QStringLiteral("complete"),
    };
    QCOMPARE(events, expected);
}

void TestDocumentSessionRouteRuntime::clearedNavigationRepublishesBeforePredecodeClear()
{
    std::vector<QString> events;
    kiriview::DocumentSessionRouteRuntimePorts ports;
    ports.cancelMediaOpenWith = []() { };
    ports.clearDirectMediaNavigation
        = [&events]() { events.push_back(QStringLiteral("clear-navigation")); };
    ports.recomputePublicProjection = [&events]() { events.push_back(QStringLiteral("publish")); };
    ports.clearMediaPredecode
        = [&events]() { events.push_back(QStringLiteral("clear-predecode")); };
    ports.routeCompleted = []() { };

    kiriview::DocumentSessionRouteRuntime runtime(std::move(ports));
    kiriview::DocumentSessionRoutePlan plan;
    plan.mutations = {
        kiriview::DocumentSessionRouteMutation {
            kiriview::ClearDirectMediaNavigationRouteOperation {} },
    };
    plan.followUpEffects = {
        kiriview::DocumentSessionRouteFollowUpEffect {
            kiriview::ClearMediaPredecodeRouteEffect {} },
    };

    runtime.execute(plan);

    const std::vector<QString> expected {
        QStringLiteral("clear-navigation"),
        QStringLiteral("clear-navigation"),
        QStringLiteral("publish"),
        QStringLiteral("clear-predecode"),
    };
    QCOMPARE(events, expected);
}

void TestDocumentSessionRouteRuntime::activeNavigationRefreshesWithoutScopeChange()
{
    int refreshCount = 0;
    kiriview::DocumentSessionRouteRuntimePorts ports;
    ports.cancelMediaOpenWith = []() { };
    ports.directMediaNavigationActive = []() { return true; };
    ports.refreshDirectMediaNavigation = [&refreshCount]() { ++refreshCount; };
    ports.routeCompleted = []() { };

    kiriview::DocumentSessionRouteRuntime runtime(std::move(ports));
    kiriview::DocumentSessionRoutePlan plan;
    plan.followUpEffects = {
        kiriview::DocumentSessionRouteFollowUpEffect {
            kiriview::RefreshDirectMediaNavigationAfterRoutingRouteEffect {} },
    };

    runtime.execute(plan);

    QCOMPARE(refreshCount, 1);
}

QTEST_GUILESS_MAIN(TestDocumentSessionRouteRuntime)

#include "test_documentsessionrouteruntime.moc"
