// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmedianavigationapplicationruntime.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <vector>

class TestDocumentSessionDirectMediaNavigationApplicationRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void inactiveRefreshClearsNavigationPublishesAndClearsPredecode();
    void inactiveImageDocumentScopeRefreshKeepsPredecodeCache();
    void failedRefreshPublishesUnknownNavigationAndReveal();
    void successfulRefreshPublishesNavigationRevealAndPredecode();
    void successfulOpenSchedulesPredecodeBeforeRoutingTarget();
};

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
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

struct AppliedNavigation {
    kiriview::DirectMediaNavigationBoundaryState state;
    bool known = false;
    std::vector<kiriview::DirectMediaNavigationCandidate> candidates;
};

struct ApplicationFixture {
    enum class Event {
        SetNavigation,
        Reveal,
        Publish,
        ClearPredecode,
        Predecode,
        Route,
    };

    std::vector<Event> events;
    AppliedNavigation navigation;
    kiriview::DocumentSessionDirectMediaNavigationRevealAction revealAction
        = kiriview::DocumentSessionDirectMediaNavigationRevealAction::None;
    int clearPredecodeCount = 0;
    std::vector<kiriview::DirectMediaNavigationCandidate> predecodeCandidates;
    QUrl routeTargetUrl;
    kiriview::DocumentSessionDirectMediaNavigationApplicationRuntime runtime {
        kiriview::DocumentSessionDirectMediaNavigationApplicationPorts {
            [this](kiriview::DirectMediaNavigationBoundaryState state, bool known,
                std::vector<kiriview::DirectMediaNavigationCandidate> candidates) {
                events.push_back(Event::SetNavigation);
                navigation = AppliedNavigation { state, known, std::move(candidates) };
            },
            [this](kiriview::DocumentSessionDirectMediaNavigationRevealAction action) {
                events.push_back(Event::Reveal);
                revealAction = action;
            },
            [this]() { events.push_back(Event::Publish); },
            [this]() {
                events.push_back(Event::ClearPredecode);
                ++clearPredecodeCount;
            },
            [this](const std::vector<kiriview::DirectMediaNavigationCandidate> &candidates) {
                events.push_back(Event::Predecode);
                predecodeCandidates = candidates;
            },
            [this](const QUrl &url) {
                events.push_back(Event::Route);
                routeTargetUrl = url;
            },
        }
    };
};
}

void TestDocumentSessionDirectMediaNavigationApplicationRuntime::
    inactiveRefreshClearsNavigationPublishesAndClearsPredecode()
{
    ApplicationFixture fixture;

    fixture.runtime.applyInactiveRefresh(true);

    QCOMPARE(fixture.navigation.known, false);
    QVERIFY(fixture.navigation.candidates.empty());
    QCOMPARE(fixture.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::ProgrammaticSync);
    QCOMPARE(fixture.clearPredecodeCount, 1);
    QCOMPARE(fixture.events,
        (std::vector<ApplicationFixture::Event> { ApplicationFixture::Event::SetNavigation,
            ApplicationFixture::Event::Reveal, ApplicationFixture::Event::Publish,
            ApplicationFixture::Event::ClearPredecode }));
}

void TestDocumentSessionDirectMediaNavigationApplicationRuntime::
    inactiveImageDocumentScopeRefreshKeepsPredecodeCache()
{
    ApplicationFixture fixture;

    fixture.runtime.applyInactiveRefresh(false);

    QCOMPARE(fixture.navigation.known, false);
    QCOMPARE(fixture.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::ProgrammaticSync);
    QCOMPARE(fixture.clearPredecodeCount, 0);
    QCOMPARE(fixture.events,
        (std::vector<ApplicationFixture::Event> { ApplicationFixture::Event::SetNavigation,
            ApplicationFixture::Event::Reveal, ApplicationFixture::Event::Publish }));
}

void TestDocumentSessionDirectMediaNavigationApplicationRuntime::
    failedRefreshPublishesUnknownNavigationAndReveal()
{
    ApplicationFixture fixture;

    fixture.runtime.applyRefresh(kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        knownActiveNavigation(2, 3),
        kiriview::DocumentSessionDirectMediaNavigationRefreshResult {
            {}, {}, false, QStringLiteral("listing failed") });

    QCOMPARE(fixture.navigation.known, false);
    QVERIFY(fixture.navigation.candidates.empty());
    QCOMPARE(fixture.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::ProgrammaticSync);
    QCOMPARE(fixture.events,
        (std::vector<ApplicationFixture::Event> { ApplicationFixture::Event::SetNavigation,
            ApplicationFixture::Event::Reveal, ApplicationFixture::Event::Publish }));
    QVERIFY(fixture.predecodeCandidates.empty());
    QVERIFY(fixture.routeTargetUrl.isEmpty());
}

void TestDocumentSessionDirectMediaNavigationApplicationRuntime::
    successfulRefreshPublishesNavigationRevealAndPredecode()
{
    ApplicationFixture fixture;
    const QUrl previousUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.png"));
    const std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        directMediaNavigationCandidate(previousUrl),
        directMediaNavigationCandidate(currentUrl),
    };

    fixture.runtime.applyRefresh(kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        knownActiveNavigation(1, 2),
        kiriview::DocumentSessionDirectMediaNavigationRefreshResult {
            candidates,
            kiriview::DirectMediaNavigationBoundaryState {
                true,
                false,
                false,
                true,
                2,
                2,
            },
            true,
            QString(),
        });

    QVERIFY(fixture.navigation.known);
    QCOMPARE(fixture.navigation.state.currentNumber, 2);
    QCOMPARE(fixture.navigation.candidates.size(), std::size_t(2));
    QCOMPARE(fixture.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::UsePendingOrProgrammaticSync);
    QCOMPARE(fixture.predecodeCandidates.size(), std::size_t(2));
    QCOMPARE(fixture.events,
        (std::vector<ApplicationFixture::Event> { ApplicationFixture::Event::SetNavigation,
            ApplicationFixture::Event::Reveal, ApplicationFixture::Event::Publish,
            ApplicationFixture::Event::Predecode }));
}

void TestDocumentSessionDirectMediaNavigationApplicationRuntime::
    successfulOpenSchedulesPredecodeBeforeRoutingTarget()
{
    ApplicationFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/01.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/02.png"));
    const std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        directMediaNavigationCandidate(currentUrl),
        directMediaNavigationCandidate(nextUrl),
    };

    fixture.runtime.applyOpen(currentUrl,
        kiriview::DocumentSessionDirectMediaNavigationOpenResult {
            candidates,
            kiriview::DirectMediaNavigationOpenPlan {
                kiriview::DirectMediaNavigationBoundaryState {
                    false,
                    true,
                    true,
                    false,
                    1,
                    2,
                },
                nextUrl,
            },
            true,
            QString(),
        });

    QVERIFY(fixture.navigation.known);
    QCOMPARE(fixture.navigation.state.currentNumber, 1);
    QCOMPARE(fixture.revealAction,
        kiriview::DocumentSessionDirectMediaNavigationRevealAction::
            UsePendingOrProgrammaticSyncAndKeepPending);
    QCOMPARE(fixture.predecodeCandidates.size(), std::size_t(2));
    QCOMPARE(fixture.routeTargetUrl, nextUrl);
    QCOMPARE(fixture.events,
        (std::vector<ApplicationFixture::Event> { ApplicationFixture::Event::SetNavigation,
            ApplicationFixture::Event::Reveal, ApplicationFixture::Event::Publish,
            ApplicationFixture::Event::Predecode, ApplicationFixture::Event::Route }));
}

QTEST_GUILESS_MAIN(TestDocumentSessionDirectMediaNavigationApplicationRuntime)

#include "test_documentsessiondirectmedianavigationapplicationruntime.moc"
