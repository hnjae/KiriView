// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmedianavigationruntime.h"

#include "async/imagecallback.h"
#include "navigation/navigationlogging.h"

#include <QDebug>
#include <memory>
#include <utility>

namespace KiriView {
DocumentSessionMediaNavigationRuntime::DocumentSessionMediaNavigationRuntime(
    MediaNavigationCandidateProvider provider)
    : m_provider(mediaNavigationCandidateProviderWithDefault(std::move(provider)))
{
}

DocumentSessionMediaNavigationRuntime::~DocumentSessionMediaNavigationRuntime() { cancel(); }

void DocumentSessionMediaNavigationRuntime::loadCandidates(QObject *receiver,
    const DirectMediaScope &scope, ScopeAccepted scopeAccepted, CandidatesCallback callback)
{
    startLoad(receiver, scope, std::move(scopeAccepted), std::move(callback));
}

void DocumentSessionMediaNavigationRuntime::refresh(QObject *receiver,
    const DirectMediaScope &scope, ScopeAccepted scopeAccepted, RefreshCallback callback)
{
    startLoad(receiver, scope, std::move(scopeAccepted),
        [callback = std::move(callback), currentUrl = scope.currentUrl](
            DocumentSessionMediaNavigationCandidatesResult result) mutable {
            if (!result.succeeded) {
                invokeIfSet(callback,
                    DocumentSessionMediaNavigationRefreshResult {
                        std::move(result.candidates), {}, false, result.errorString });
                return;
            }

            invokeIfSet(callback,
                DocumentSessionMediaNavigationRefreshResult { result.candidates,
                    mediaNavigationBoundaryState(result.candidates, currentUrl), true,
                    result.errorString });
        });
}

void DocumentSessionMediaNavigationRuntime::open(QObject *receiver, const DirectMediaScope &scope,
    MediaNavigationOpenRequest request, ScopeAccepted scopeAccepted, OpenCallback callback)
{
    startLoad(receiver, scope, std::move(scopeAccepted),
        [callback = std::move(callback), currentUrl = scope.currentUrl, request](
            DocumentSessionMediaNavigationCandidatesResult result) mutable {
            if (!result.succeeded) {
                invokeIfSet(callback,
                    DocumentSessionMediaNavigationOpenResult {
                        std::move(result.candidates), {}, false, result.errorString });
                return;
            }

            invokeIfSet(callback,
                DocumentSessionMediaNavigationOpenResult { result.candidates,
                    mediaNavigationOpenPlan(result.candidates, currentUrl, request), true,
                    result.errorString });
        });
}

void DocumentSessionMediaNavigationRuntime::startLoad(QObject *receiver,
    const DirectMediaScope &scope, ScopeAccepted scopeAccepted, CandidatesCallback callback)
{
    cancel();

    if (scope.currentUrl.isEmpty() || scope.parentUrl.isEmpty() || !scope.parentUrl.isValid()
        || !m_provider.directoryMedia) {
        qCDebug(kiriviewNavigationLog)
            << "media navigation candidate load skipped"
            << "reason"
            << "invalid-scope"
            << "currentUrl" << scope.currentUrl << "parentUrl" << scope.parentUrl << "generation"
            << scope.generation << "providerPresent"
            << static_cast<bool>(m_provider.directoryMedia);
        invokeIfSet(callback, DocumentSessionMediaNavigationCandidatesResult {});
        return;
    }

    const DocumentSessionMediaNavigationLoad load = m_loadState.start(scope);
    qCDebug(kiriviewNavigationLog)
        << "media navigation candidate load started"
        << "operationId" << load.operationId << "currentUrl" << scope.currentUrl << "parentUrl"
        << scope.parentUrl << "generation" << scope.generation;
    auto sharedScopeAccepted = std::make_shared<ScopeAccepted>(std::move(scopeAccepted));
    auto sharedCallback = std::make_shared<CandidatesCallback>(std::move(callback));

    m_job = m_provider.directoryMedia(
        receiver, scope.parentUrl,
        [this, load, sharedScopeAccepted, sharedCallback](
            std::vector<MediaNavigationCandidate> candidates) mutable {
            finish(load,
                DocumentSessionMediaNavigationCandidatesResult {
                    std::move(candidates), true, QString() },
                *sharedScopeAccepted, *sharedCallback);
        },
        [this, load, sharedScopeAccepted, sharedCallback](const QString &errorString) {
            finish(load, DocumentSessionMediaNavigationCandidatesResult { {}, false, errorString },
                *sharedScopeAccepted, *sharedCallback);
        });
}

void DocumentSessionMediaNavigationRuntime::cancel()
{
    m_job.cancel();
    m_loadState.cancel();
}

void DocumentSessionMediaNavigationRuntime::finish(DocumentSessionMediaNavigationLoad load,
    DocumentSessionMediaNavigationCandidatesResult result, const ScopeAccepted &scopeAccepted,
    const CandidatesCallback &callback)
{
    if (!m_loadState.finish(load)) {
        qCDebug(kiriviewNavigationLog)
            << "media navigation candidate load ignored"
            << "reason"
            << "stale-load"
            << "operationId" << load.operationId << "currentUrl" << load.scope.currentUrl
            << "parentUrl" << load.scope.parentUrl << "generation" << load.scope.generation;
        return;
    }

    if (scopeAccepted && !scopeAccepted(load.scope)) {
        qCDebug(kiriviewNavigationLog)
            << "media navigation candidate load ignored"
            << "reason"
            << "scope-rejected"
            << "operationId" << load.operationId << "currentUrl" << load.scope.currentUrl
            << "parentUrl" << load.scope.parentUrl << "generation" << load.scope.generation;
        return;
    }

    qCDebug(kiriviewNavigationLog)
        << "media navigation candidate load finished"
        << "operationId" << load.operationId << "succeeded" << result.succeeded << "candidates"
        << result.candidates.size() << "error" << result.errorString;
    invokeIfSet(callback, std::move(result));
}
}
