// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIANAVIGATIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIANAVIGATIONRUNTIME_H

#include "navigation/mediacandidateprovider.h"
#include "navigation/medianavigationmodel.h"
#include "session/documentsessionmedianavigationloadstate.h"

#include <QString>
#include <functional>
#include <vector>

class QObject;

namespace KiriView {
struct DocumentSessionMediaNavigationCandidatesResult {
    std::vector<MediaNavigationCandidate> candidates;
    bool succeeded = false;
    QString errorString;
};

struct DocumentSessionMediaNavigationRefreshResult {
    std::vector<MediaNavigationCandidate> candidates;
    MediaNavigationBoundaryState boundaryState;
    bool succeeded = false;
    QString errorString;
};

struct DocumentSessionMediaNavigationOpenResult {
    std::vector<MediaNavigationCandidate> candidates;
    MediaNavigationOpenPlan plan;
    bool succeeded = false;
    QString errorString;
};

class DocumentSessionMediaNavigationRuntime final
{
public:
    using ScopeAccepted = std::function<bool(const DocumentSessionMediaNavigationLoadScope &)>;
    using CandidatesCallback = std::function<void(DocumentSessionMediaNavigationCandidatesResult)>;
    using RefreshCallback = std::function<void(DocumentSessionMediaNavigationRefreshResult)>;
    using OpenCallback = std::function<void(DocumentSessionMediaNavigationOpenResult)>;

    explicit DocumentSessionMediaNavigationRuntime(MediaNavigationCandidateProvider provider = {});
    ~DocumentSessionMediaNavigationRuntime();

    void loadCandidates(QObject *receiver, const DocumentSessionMediaNavigationLoadScope &scope,
        ScopeAccepted scopeAccepted, CandidatesCallback callback);
    void refresh(QObject *receiver, const DocumentSessionMediaNavigationLoadScope &scope,
        ScopeAccepted scopeAccepted, RefreshCallback callback);
    void open(QObject *receiver, const DocumentSessionMediaNavigationLoadScope &scope,
        MediaNavigationOpenRequest request, ScopeAccepted scopeAccepted, OpenCallback callback);
    void cancel();

private:
    void startLoad(QObject *receiver, const DocumentSessionMediaNavigationLoadScope &scope,
        ScopeAccepted scopeAccepted, CandidatesCallback callback);
    void finish(DocumentSessionMediaNavigationLoad load,
        DocumentSessionMediaNavigationCandidatesResult result, const ScopeAccepted &scopeAccepted,
        const CandidatesCallback &callback);

    MediaNavigationCandidateProvider m_provider;
    ImageIoJob m_job;
    DocumentSessionMediaNavigationLoadState m_loadState;
};
}

#endif
