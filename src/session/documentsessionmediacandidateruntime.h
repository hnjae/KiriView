// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIACANDIDATERUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIACANDIDATERUNTIME_H

#include "navigation/mediacandidateprovider.h"
#include "session/documentsessionmediacandidateloadstate.h"

#include <QString>
#include <functional>
#include <vector>

class QObject;

namespace KiriView {
struct MediaCandidateLoadResult {
    std::vector<MediaNavigationCandidate> candidates;
    bool succeeded = false;
    QString errorString;
};

class DocumentSessionMediaCandidateRuntime final
{
public:
    using ScopeAccepted = std::function<bool(const DocumentSessionMediaCandidateLoadScope &)>;
    using CompletionCallback = std::function<void(MediaCandidateLoadResult)>;

    explicit DocumentSessionMediaCandidateRuntime(MediaNavigationCandidateProvider provider = {});
    ~DocumentSessionMediaCandidateRuntime();

    void load(QObject *receiver, const DocumentSessionMediaCandidateLoadScope &scope,
        ScopeAccepted scopeAccepted, CompletionCallback callback);
    void cancel();

private:
    void finish(DocumentSessionMediaCandidateLoad load, MediaCandidateLoadResult result,
        const ScopeAccepted &scopeAccepted, const CompletionCallback &callback);

    MediaNavigationCandidateProvider m_provider;
    ImageIoJob m_job;
    DocumentSessionMediaCandidateLoadState m_loadState;
};
}

#endif
