// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmedianavigationruntime.h"

#include "async/imagecallback.h"
#include "navigation/navigationlogging.h"

#include <QDebug>
#include <memory>
#include <utility>

namespace KiriView {
DocumentSessionDirectMediaNavigationRuntime::DocumentSessionDirectMediaNavigationRuntime(
    DirectMediaNavigationCandidateProvider provider)
    : m_provider(directMediaNavigationCandidateProviderWithDefault(std::move(provider)))
{
}

DocumentSessionDirectMediaNavigationRuntime::~DocumentSessionDirectMediaNavigationRuntime()
{
    cancel();
}

void DocumentSessionDirectMediaNavigationRuntime::loadCandidates(QObject *receiver,
    const DirectMediaScope &scope, ScopeAccepted scopeAccepted, CandidatesCallback callback)
{
    startLoad(receiver, scope, std::move(scopeAccepted), std::move(callback));
}

void DocumentSessionDirectMediaNavigationRuntime::refresh(QObject *receiver,
    const DirectMediaScope &scope, ScopeAccepted scopeAccepted, RefreshCallback callback)
{
    startLoad(receiver, scope, std::move(scopeAccepted),
        [callback = std::move(callback), currentUrl = scope.currentUrl](
            DocumentSessionDirectMediaNavigationCandidatesResult result) mutable {
            if (!result.succeeded) {
                invokeIfSet(callback,
                    DocumentSessionDirectMediaNavigationRefreshResult {
                        std::move(result.candidates), {}, false, result.errorString });
                return;
            }

            invokeIfSet(callback,
                DocumentSessionDirectMediaNavigationRefreshResult { result.candidates,
                    directMediaNavigationBoundaryState(result.candidates, currentUrl), true,
                    result.errorString });
        });
}

void DocumentSessionDirectMediaNavigationRuntime::open(QObject *receiver,
    const DirectMediaScope &scope, DirectMediaNavigationOpenRequest request,
    ScopeAccepted scopeAccepted, OpenCallback callback)
{
    startLoad(receiver, scope, std::move(scopeAccepted),
        [callback = std::move(callback), currentUrl = scope.currentUrl, request](
            DocumentSessionDirectMediaNavigationCandidatesResult result) mutable {
            if (!result.succeeded) {
                invokeIfSet(callback,
                    DocumentSessionDirectMediaNavigationOpenResult {
                        std::move(result.candidates), {}, false, result.errorString });
                return;
            }

            invokeIfSet(callback,
                DocumentSessionDirectMediaNavigationOpenResult { result.candidates,
                    directMediaNavigationOpenPlan(result.candidates, currentUrl, request), true,
                    result.errorString });
        });
}

void DocumentSessionDirectMediaNavigationRuntime::startLoad(QObject *receiver,
    const DirectMediaScope &scope, ScopeAccepted scopeAccepted, CandidatesCallback callback)
{
    cancel();

    if (scope.currentUrl.isEmpty() || scope.parentUrl.isEmpty() || !scope.parentUrl.isValid()
        || !m_provider.directoryCandidateLoader) {
        qCDebug(kiriviewNavigationLog)
            << "direct media navigation candidate load skipped"
            << "reason"
            << "invalid-scope"
            << "currentUrl" << scope.currentUrl << "parentUrl" << scope.parentUrl << "generation"
            << scope.generation << "providerPresent"
            << static_cast<bool>(m_provider.directoryCandidateLoader);
        invokeIfSet(callback, DocumentSessionDirectMediaNavigationCandidatesResult {});
        return;
    }

    const DocumentSessionDirectMediaNavigationLoad load = m_loadState.start(scope);
    qCDebug(kiriviewNavigationLog)
        << "direct media navigation candidate load started"
        << "operationId" << load.operationId << "currentUrl" << scope.currentUrl << "parentUrl"
        << scope.parentUrl << "generation" << scope.generation;
    auto sharedScopeAccepted = std::make_shared<ScopeAccepted>(std::move(scopeAccepted));
    auto sharedCallback = std::make_shared<CandidatesCallback>(std::move(callback));

    m_job = m_provider.directoryCandidateLoader(
        receiver, scope.parentUrl,
        [this, load, sharedScopeAccepted, sharedCallback](
            std::vector<DirectMediaNavigationCandidate> candidates) mutable {
            finish(load,
                DocumentSessionDirectMediaNavigationCandidatesResult {
                    std::move(candidates), true, QString() },
                *sharedScopeAccepted, *sharedCallback);
        },
        [this, load, sharedScopeAccepted, sharedCallback](const QString &errorString) {
            finish(load,
                DocumentSessionDirectMediaNavigationCandidatesResult { {}, false, errorString },
                *sharedScopeAccepted, *sharedCallback);
        });
}

void DocumentSessionDirectMediaNavigationRuntime::cancel()
{
    m_job.cancel();
    m_loadState.cancel();
}

void DocumentSessionDirectMediaNavigationRuntime::finish(
    DocumentSessionDirectMediaNavigationLoad load,
    DocumentSessionDirectMediaNavigationCandidatesResult result, const ScopeAccepted &scopeAccepted,
    const CandidatesCallback &callback)
{
    if (!m_loadState.finish(load)) {
        qCDebug(kiriviewNavigationLog)
            << "direct media navigation candidate load ignored"
            << "reason"
            << "stale-load"
            << "operationId" << load.operationId << "currentUrl" << load.scope.currentUrl
            << "parentUrl" << load.scope.parentUrl << "generation" << load.scope.generation;
        return;
    }

    if (scopeAccepted && !scopeAccepted(load.scope)) {
        qCDebug(kiriviewNavigationLog)
            << "direct media navigation candidate load ignored"
            << "reason"
            << "scope-rejected"
            << "operationId" << load.operationId << "currentUrl" << load.scope.currentUrl
            << "parentUrl" << load.scope.parentUrl << "generation" << load.scope.generation;
        return;
    }

    qCDebug(kiriviewNavigationLog)
        << "direct media navigation candidate load finished"
        << "operationId" << load.operationId << "succeeded" << result.succeeded << "candidates"
        << result.candidates.size() << "error" << result.errorString;
    invokeIfSet(callback, std::move(result));
}
}
