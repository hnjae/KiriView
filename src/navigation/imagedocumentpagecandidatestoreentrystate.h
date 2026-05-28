// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATESTOREENTRYSTATE_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATESTOREENTRYSTATE_H

#include "async/imageasynccallbacks.h"
#include "async/imageiojob.h"
#include "imagedocumentpagecandidatecallbacks.h"
#include "imagedocumentpagenavigationtypes.h"

#include <QPointer>
#include <QString>
#include <vector>

class QObject;

namespace KiriView {
struct ImageDocumentPageCandidateStoreEntryPendingLoad {
    ImageIoJobCompletion completion;
    ImageDocumentPageCandidatesCallback callback;
    ErrorCallback errorCallback;
};

struct ImageDocumentPageCandidateStoreEntrySubscriber {
    QPointer<QObject> token;
    ImageDocumentPageCandidatesCallback callback;
    ErrorCallback errorCallback;
};

struct ImageDocumentPageCandidateStoreEntryNotificationPlan {
    std::vector<ImageDocumentPageCandidateStoreEntryPendingLoad> completedLoads;
    std::vector<ImageDocumentPageCandidateStoreEntryPendingLoad> failedLoads;
    std::vector<ImageDocumentPageCandidateStoreEntrySubscriber> changedSubscribers;
    std::vector<ImageDocumentPageCandidateStoreEntrySubscriber> failedSubscribers;
    std::vector<ImageDocumentPageCandidate> candidates;
    QString errorString;
};

class ImageDocumentPageCandidateStoreEntryState final
{
public:
    const std::vector<ImageDocumentPageCandidate> &candidates() const;
    bool listed() const;
    bool failed() const;
    const QString &errorString() const;

    void addPendingLoad(ImageIoJobCompletion completion,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback);
    void addSubscriber(
        QObject *token, ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback);
    void removePendingLoad(QObject *token);
    void removeSubscriber(QObject *token);

    ImageDocumentPageCandidateStoreEntryNotificationPlan completeListing(
        std::vector<ImageDocumentPageCandidate> candidates);
    ImageDocumentPageCandidateStoreEntryNotificationPlan updateListing(
        std::vector<ImageDocumentPageCandidate> candidates);
    ImageDocumentPageCandidateStoreEntryNotificationPlan failListing(QString errorString);

private:
    bool replaceCandidates(std::vector<ImageDocumentPageCandidate> candidates);
    std::vector<ImageDocumentPageCandidateStoreEntryPendingLoad> takePendingLoads();
    std::vector<ImageDocumentPageCandidateStoreEntrySubscriber> activeSubscribers();

    std::vector<ImageDocumentPageCandidate> m_candidates;
    std::vector<ImageDocumentPageCandidateStoreEntryPendingLoad> m_pendingLoads;
    std::vector<ImageDocumentPageCandidateStoreEntrySubscriber> m_subscribers;
    bool m_listed = false;
    bool m_failed = false;
    QString m_errorString;
};
}

#endif
