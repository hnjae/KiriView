// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEDIRECTORYENTRY_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEDIRECTORYENTRY_H

#include "async/imageasynccallbacks.h"
#include "imagedocumentpagecandidatecallbacks.h"
#include "imagedocumentpagecandidatestoreentrystate.h"
#include "imagedocumentpagecandidatewatchprovider.h"
#include "imagedocumentpagenavigationtypes.h"

#include <QList>
#include <QString>
#include <QUrl>
#include <functional>
#include <vector>

class QObject;

namespace kiriview {
class ImageDocumentPageCandidateDirectoryEntry final
{
public:
    ImageDocumentPageCandidateDirectoryEntry(QUrl directoryUrl,
        ImageDocumentPageCandidateWatchProvider watchProvider, QObject *signalContext);
    ~ImageDocumentPageCandidateDirectoryEntry();

    bool failed() const;
    bool listed() const;
    const QString &errorString() const;
    const std::vector<ImageDocumentPageCandidate> &candidates() const;

    bool open();
    void handleCompleted(std::vector<ImageDocumentPageCandidate> candidates);
    void handleChanged(std::vector<ImageDocumentPageCandidate> candidates);
    void handleDeleted(const QList<QUrl> &urls);
    void handleError(const QString &errorString);

    ImageIoJob addPendingLoad(ImageDocumentPageCandidatesCallback callback,
        ErrorCallback errorCallback, QObject *receiver, std::function<void(QObject *)> removeToken);
    ImageIoJob addSubscriber(ImageDocumentPageCandidatesCallback callback,
        ErrorCallback errorCallback, QObject *receiver, std::function<void(QObject *)> removeToken);
    void removePendingLoad(QObject *token);
    void removeSubscriber(QObject *token);

private:
    QUrl m_directoryUrl;
    ImageDocumentPageCandidateWatchProvider m_watchProvider;
    QObject *m_signalContext = nullptr;
    ImageIoJob m_watchJob;
    ImageDocumentPageCandidateStoreEntryState m_state;
};
}

#endif
