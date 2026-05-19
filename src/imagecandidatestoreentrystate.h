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

    void completeListing(std::vector<ImageNavigationCandidate> candidates);
    void updateListing(std::vector<ImageNavigationCandidate> candidates);
    void failListing(QString errorString);

private:
    struct PendingLoad {
        ImageIoJobCompletion completion;
        ImageCandidatesCallback callback;
        ErrorCallback errorCallback;
    };

    struct Subscriber {
        QPointer<QObject> token;
        ImageCandidatesCallback callback;
        ErrorCallback errorCallback;
    };

    bool replaceCandidates(std::vector<ImageNavigationCandidate> candidates);
    void finishPendingLoads();
    void finishPendingLoadErrors();
    void notifySubscribers();
    void notifySubscriberErrors();

    std::vector<ImageNavigationCandidate> m_candidates;
    std::vector<PendingLoad> m_pendingLoads;
    std::vector<Subscriber> m_subscribers;
    bool m_listed = false;
    bool m_failed = false;
    QString m_errorString;
};
}

#endif
