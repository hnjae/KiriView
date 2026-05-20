// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATESTOREENTRYSTATE_H
#define KIRIVIEW_IMAGECANDIDATESTOREENTRYSTATE_H

#include "imageasynccallbacks.h"
#include "imageiojob.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <functional>
#include <vector>

namespace KiriView {
struct ImageCandidateStoreEntryPendingLoad {
    ImageIoJobCompletion completion;
    ImageCandidatesCallback callback;
    ErrorCallback errorCallback;
};

struct ImageCandidateStoreEntrySubscriber {
    QPointer<QObject> token;
    ImageCandidatesCallback callback;
    ErrorCallback errorCallback;
};

struct ImageCandidateStoreEntryNotificationPlan {
    std::vector<ImageCandidateStoreEntryPendingLoad> completedLoads;
    std::vector<ImageCandidateStoreEntryPendingLoad> failedLoads;
    std::vector<ImageCandidateStoreEntrySubscriber> changedSubscribers;
    std::vector<ImageCandidateStoreEntrySubscriber> failedSubscribers;
    std::vector<ImageNavigationCandidate> candidates;
    QString errorString;
};

class ImageCandidateStoreEntryState final
{
public:
    const std::vector<ImageNavigationCandidate> &candidates() const;
    bool listed() const;
    bool failed() const;
    const QString &errorString() const;

    ImageIoJob addPendingLoad(QObject *receiver, QObject *fallbackParent,
        ImageCandidatesCallback callback, ErrorCallback errorCallback,
        std::function<void(QObject *)> removeToken);
    ImageIoJob addSubscriber(QObject *receiver, QObject *fallbackParent,
        ImageCandidatesCallback callback, ErrorCallback errorCallback,
        std::function<void(QObject *)> removeToken);
    void removePendingLoad(QObject *token);
    void removeSubscriber(QObject *token);

    ImageCandidateStoreEntryNotificationPlan completeListing(
        std::vector<ImageNavigationCandidate> candidates);
    ImageCandidateStoreEntryNotificationPlan updateListing(
        std::vector<ImageNavigationCandidate> candidates);
    ImageCandidateStoreEntryNotificationPlan failListing(QString errorString);

private:
    bool replaceCandidates(std::vector<ImageNavigationCandidate> candidates);
    std::vector<ImageCandidateStoreEntryPendingLoad> takePendingLoads();
    std::vector<ImageCandidateStoreEntrySubscriber> activeSubscribers();

    std::vector<ImageNavigationCandidate> m_candidates;
    std::vector<ImageCandidateStoreEntryPendingLoad> m_pendingLoads;
    std::vector<ImageCandidateStoreEntrySubscriber> m_subscribers;
    bool m_listed = false;
    bool m_failed = false;
    QString m_errorString;
};
}

#endif
