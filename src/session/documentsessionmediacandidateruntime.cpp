// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediacandidateruntime.h"

#include "async/imagecallback.h"

#include <memory>
#include <utility>

namespace KiriView {
DocumentSessionMediaCandidateRuntime::DocumentSessionMediaCandidateRuntime(
    MediaNavigationCandidateProvider provider)
    : m_provider(mediaNavigationCandidateProviderWithDefault(std::move(provider)))
{
}

DocumentSessionMediaCandidateRuntime::~DocumentSessionMediaCandidateRuntime() { cancel(); }

void DocumentSessionMediaCandidateRuntime::load(QObject *receiver,
    const DocumentSessionMediaCandidateLoadScope &scope, ScopeAccepted scopeAccepted,
    CompletionCallback callback)
{
    cancel();

    if (scope.currentUrl.isEmpty() || scope.parentUrl.isEmpty() || !scope.parentUrl.isValid()
        || !m_provider.directoryMedia) {
        invokeIfSet(callback, MediaCandidateLoadResult {});
        return;
    }

    const DocumentSessionMediaCandidateLoad load
        = m_loadState.start(scope.currentUrl, scope.parentUrl, scope.cursorGeneration);
    auto sharedScopeAccepted = std::make_shared<ScopeAccepted>(std::move(scopeAccepted));
    auto sharedCallback = std::make_shared<CompletionCallback>(std::move(callback));

    m_job = m_provider.directoryMedia(
        receiver, scope.parentUrl,
        [this, load, sharedScopeAccepted, sharedCallback](
            std::vector<MediaNavigationCandidate> candidates) mutable {
            finish(load, MediaCandidateLoadResult { std::move(candidates), true, QString() },
                *sharedScopeAccepted, *sharedCallback);
        },
        [this, load, sharedScopeAccepted, sharedCallback](const QString &errorString) {
            finish(load, MediaCandidateLoadResult { {}, false, errorString }, *sharedScopeAccepted,
                *sharedCallback);
        });
}

void DocumentSessionMediaCandidateRuntime::cancel()
{
    m_job.cancel();
    m_loadState.cancel();
}

void DocumentSessionMediaCandidateRuntime::finish(DocumentSessionMediaCandidateLoad load,
    MediaCandidateLoadResult result, const ScopeAccepted &scopeAccepted,
    const CompletionCallback &callback)
{
    if (!m_loadState.finish(load)) {
        return;
    }

    const DocumentSessionMediaCandidateLoadScope scope {
        load.currentUrl,
        load.parentUrl,
        load.cursorGeneration,
    };
    if (scopeAccepted && !scopeAccepted(scope)) {
        return;
    }

    invokeIfSet(callback, std::move(result));
}
}
