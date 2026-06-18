// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmedianavigationcoordinator.h"

#include "image_async_test_support.h"

#include <QObject>
#include <QTest>
#include <memory>
#include <utility>
#include <vector>

class TestDocumentSessionDirectMediaNavigationCoordinator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inactiveRefreshAppliesInactiveNavigationWithoutLoadingCandidates();
    void activeRefreshLoadsCandidatesAndAppliesNavigation();
    void activeOpenRoutesTargetUsingCurrentCursor();
};

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

kiriview::DirectMediaScope directMediaScope(const QUrl &currentUrl)
{
    return kiriview::DirectMediaScope {
        currentUrl,
        localUrl(QStringLiteral("/media/")),
        7,
    };
}

kiriview::ActiveNavigationSnapshot knownActiveNavigation(int currentNumber, int count)
{
    return kiriview::ActiveNavigationSnapshot {
        true,
        true,
        true,
        currentNumber > 1,
        currentNumber < count,
        currentNumber == 1,
        currentNumber == count,
        currentNumber,
        count,
    };
}

struct ManualDirectMediaNavigationCandidateLoad {
    QUrl parentUrl;
    kiriview::DirectMediaNavigationCandidatesCallback callback;
    kiriview::ErrorCallback errorCallback;
};

class ManualDirectMediaNavigationCandidateProvider
{
public:
    kiriview::DirectMediaNavigationCandidateProvider provider()
    {
        return kiriview::DirectMediaNavigationCandidateProvider {
            [this](QObject *receiver, QUrl parentUrl,
                kiriview::DirectMediaNavigationCandidatesCallback callback,
                kiriview::ErrorCallback errorCallback) {
                auto load = std::make_shared<ManualDirectMediaNavigationCandidateLoad>();
                load->parentUrl = std::move(parentUrl);
                load->callback = std::move(callback);
                load->errorCallback = std::move(errorCallback);
                m_loads.push_back(load);
                return kiriview::TestSupport::Detail::startManualIoJob(receiver, load);
            },
        };
    }

    std::size_t loadCount() const { return m_loads.size(); }

    ManualDirectMediaNavigationCandidateLoad &loadAt(std::size_t index)
    {
        return *m_loads.at(index);
    }

    void deliver(
        std::size_t index, std::vector<kiriview::DirectMediaNavigationCandidate> candidates)
    {
        ManualDirectMediaNavigationCandidateLoad &load = loadAt(index);
        if (load.callback) {
            load.callback(std::move(candidates));
        }
    }

private:
    std::vector<std::shared_ptr<ManualDirectMediaNavigationCandidateLoad>> m_loads;
};

struct AppliedNavigation {
    kiriview::DirectMediaNavigationBoundaryState state;
    bool known = false;
    std::vector<kiriview::DirectMediaNavigationCandidate> candidates;
};

struct CoordinatorFixture {
    enum class Event {
        SetNavigation,
        Reveal,
        Publish,
        ClearPredecode,
        Predecode,
        Route,
    };

    QObject receiver;
    ManualDirectMediaNavigationCandidateProvider provider;
    bool navigationActive = true;
    bool directImageSourceScopeEligible = true;
    kiriview::DirectMediaScope scope = directMediaScope(localUrl(QStringLiteral("/media/02.png")));
    QUrl activeCursorUrl = localUrl(QStringLiteral("/media/02.png"));
    kiriview::ActiveNavigationSourceKind activeNavigationSourceKind
        = kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia;
    kiriview::ActiveNavigationSnapshot activeNavigation = knownActiveNavigation(2, 3);
    std::vector<Event> events;
    AppliedNavigation navigation;
    kiriview::DocumentSessionDirectMediaNavigationRevealAction revealAction
        = kiriview::DocumentSessionDirectMediaNavigationRevealAction::None;
    int clearPredecodeCount = 0;
    std::vector<kiriview::DirectMediaNavigationCandidate> predecodeCandidates;
    QUrl routeTargetUrl;
    std::unique_ptr<kiriview::DocumentSessionDirectMediaNavigationCoordinator> coordinator;

    CoordinatorFixture()
    {
        kiriview::DocumentSessionDirectMediaNavigationCoordinatorPorts ports;
        ports.navigationActive = [this]() { return navigationActive; };
        ports.directImageSourceScopeEligible = [this]() { return directImageSourceScopeEligible; };
        ports.currentScope = [this]() { return scope; };
        ports.cursorMatches = [this](const kiriview::DirectMediaScope &candidateScope) {
            return candidateScope.currentUrl == scope.currentUrl
                && candidateScope.parentUrl == scope.parentUrl
                && candidateScope.generation == scope.generation;
        };
        ports.activeCursorUrl = [this]() { return activeCursorUrl; };
        ports.activeNavigationSourceKind = [this]() { return activeNavigationSourceKind; };
        ports.activeNavigationSnapshot = [this]() { return activeNavigation; };
        ports.setDirectMediaNavigation
            = [this](kiriview::DirectMediaNavigationBoundaryState state, bool known,
                  std::vector<kiriview::DirectMediaNavigationCandidate> candidates) {
                  events.push_back(Event::SetNavigation);
                  navigation = AppliedNavigation { state, known, std::move(candidates) };
              };
        ports.applyRevealAction
            = [this](kiriview::DocumentSessionDirectMediaNavigationRevealAction action) {
                  events.push_back(Event::Reveal);
                  revealAction = action;
              };
        ports.recomputePublicProjection = [this]() { events.push_back(Event::Publish); };
        ports.clearPredecode = [this]() {
            events.push_back(Event::ClearPredecode);
            ++clearPredecodeCount;
        };
        ports.schedulePredecode
            = [this](const std::vector<kiriview::DirectMediaNavigationCandidate> &candidates) {
                  events.push_back(Event::Predecode);
                  predecodeCandidates = candidates;
              };
        ports.openMediaUrl = [this](const QUrl &url) {
            events.push_back(Event::Route);
            routeTargetUrl = url;
        };
        coordinator = std::make_unique<kiriview::DocumentSessionDirectMediaNavigationCoordinator>(
            provider.provider(), std::move(ports));
    }
};
}

void TestDocumentSessionDirectMediaNavigationCoordinator::
    inactiveRefreshAppliesInactiveNavigationWithoutLoadingCandidates()
{
    CoordinatorFixture fixture;
    fixture.navigationActive = false;
    fixture.directImageSourceScopeEligible = false;

    fixture.coordinator->refresh(&fixture.receiver);

    QCOMPARE(fixture.provider.loadCount(), std::size_t(0));
    QVERIFY(!fixture.navigation.known);
    QVERIFY(fixture.navigation.candidates.empty());
    QCOMPARE(fixture.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::ProgrammaticSync);
    QCOMPARE(fixture.clearPredecodeCount, 1);
    QCOMPARE(fixture.events,
        (std::vector<CoordinatorFixture::Event> { CoordinatorFixture::Event::SetNavigation,
            CoordinatorFixture::Event::Reveal, CoordinatorFixture::Event::Publish,
            CoordinatorFixture::Event::ClearPredecode }));
}

void TestDocumentSessionDirectMediaNavigationCoordinator::
    activeRefreshLoadsCandidatesAndAppliesNavigation()
{
    CoordinatorFixture fixture;
    const QUrl previousUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/03.png"));

    fixture.coordinator->refresh(&fixture.receiver);
    fixture.provider.deliver(0,
        { directMediaNavigationCandidate(previousUrl), directMediaNavigationCandidate(currentUrl),
            directMediaNavigationCandidate(nextUrl) });

    QCOMPARE(fixture.provider.loadAt(0).parentUrl, localUrl(QStringLiteral("/media/")));
    QVERIFY(fixture.navigation.known);
    QCOMPARE(fixture.navigation.state.currentNumber, 2);
    QCOMPARE(fixture.navigation.state.count, 3);
    QCOMPARE(fixture.predecodeCandidates.size(), std::size_t(3));
    QCOMPARE(fixture.events,
        (std::vector<CoordinatorFixture::Event> { CoordinatorFixture::Event::SetNavigation,
            CoordinatorFixture::Event::Reveal, CoordinatorFixture::Event::Publish,
            CoordinatorFixture::Event::Predecode }));
}

void TestDocumentSessionDirectMediaNavigationCoordinator::activeOpenRoutesTargetUsingCurrentCursor()
{
    CoordinatorFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/03.png"));

    fixture.coordinator->openNext(&fixture.receiver);
    fixture.provider.deliver(
        0, { directMediaNavigationCandidate(currentUrl), directMediaNavigationCandidate(nextUrl) });

    QCOMPARE(fixture.routeTargetUrl, nextUrl);
    QCOMPARE(fixture.predecodeCandidates.size(), std::size_t(2));
    QCOMPARE(fixture.events,
        (std::vector<CoordinatorFixture::Event> { CoordinatorFixture::Event::SetNavigation,
            CoordinatorFixture::Event::Reveal, CoordinatorFixture::Event::Publish,
            CoordinatorFixture::Event::Predecode, CoordinatorFixture::Event::Route }));
}

QTEST_GUILESS_MAIN(TestDocumentSessionDirectMediaNavigationCoordinator)
#include "test_documentsessiondirectmedianavigationcoordinator.moc"
