// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONRUNTIME_H

#include "navigation/directmedianavigationcandidateprovider.h"
#include "navigation/directmedianavigationmodel.h"
#include "session/documentsessiondirectmedianavigationloadstate.h"

#include <QString>
#include <functional>
#include <vector>

class QObject;

namespace kiriview {
struct DocumentSessionDirectMediaNavigationCandidatesResult {
    std::vector<DirectMediaNavigationCandidate> candidates;
    bool succeeded = false;
    QString errorString;
};

struct DocumentSessionDirectMediaNavigationRefreshResult {
    std::vector<DirectMediaNavigationCandidate> candidates;
    DirectMediaNavigationBoundaryState boundaryState;
    bool succeeded = false;
    QString errorString;
};

struct DocumentSessionDirectMediaNavigationOpenResult {
    std::vector<DirectMediaNavigationCandidate> candidates;
    DirectMediaNavigationOpenPlan plan;
    bool succeeded = false;
    QString errorString;
};

class DocumentSessionDirectMediaNavigationRuntime final
{
public:
    using ScopeAccepted = std::function<bool(const DirectMediaScope &)>;
    using CandidatesCallback
        = std::function<void(DocumentSessionDirectMediaNavigationCandidatesResult)>;
    using RefreshCallback = std::function<void(DocumentSessionDirectMediaNavigationRefreshResult)>;
    using OpenCallback = std::function<void(DocumentSessionDirectMediaNavigationOpenResult)>;

    explicit DocumentSessionDirectMediaNavigationRuntime(
        DirectMediaNavigationCandidateProvider provider = {});
    ~DocumentSessionDirectMediaNavigationRuntime();

    void loadCandidates(QObject *receiver, const DirectMediaScope &scope,
        ScopeAccepted scopeAccepted, CandidatesCallback callback);
    void refresh(QObject *receiver, const DirectMediaScope &scope, ScopeAccepted scopeAccepted,
        RefreshCallback callback);
    void open(QObject *receiver, const DirectMediaScope &scope,
        DirectMediaNavigationOpenRequest request, ScopeAccepted scopeAccepted,
        OpenCallback callback);
    void cancel();

private:
    void startLoad(QObject *receiver, const DirectMediaScope &scope, ScopeAccepted scopeAccepted,
        CandidatesCallback callback);
    void finish(DocumentSessionDirectMediaNavigationLoad load,
        DocumentSessionDirectMediaNavigationCandidatesResult result,
        const ScopeAccepted &scopeAccepted, const CandidatesCallback &callback);

    DirectMediaNavigationCandidateProvider m_provider;
    ImageIoJob m_job;
    DocumentSessionDirectMediaNavigationLoadState m_loadState;
};
}

#endif
