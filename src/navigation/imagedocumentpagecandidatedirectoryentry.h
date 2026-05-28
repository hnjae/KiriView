// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEDIRECTORYENTRY_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATEDIRECTORYENTRY_H

#include "async/imageasynccallbacks.h"
#include "imagedocumentpagecandidatecallbacks.h"
#include "imagedocumentpagecandidatestoreentrystate.h"
#include "imagedocumentpagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <vector>

class KCoreDirLister;
class QObject;

namespace KiriView {
class ImageDocumentPageCandidateDirectoryEntry final
{
public:
    ImageDocumentPageCandidateDirectoryEntry(QUrl directoryUrl, QObject *signalContext);
    ~ImageDocumentPageCandidateDirectoryEntry();

    bool failed() const;
    bool listed() const;
    const QString &errorString() const;
    const std::vector<ImageDocumentPageCandidate> &candidates() const;

    bool open();
    void handleCompleted();
    void handleChanged();
    void handleError(const QString &errorString);

    ImageIoJob addPendingLoad(ImageDocumentPageCandidatesCallback callback,
        ErrorCallback errorCallback, QObject *receiver, std::function<void(QObject *)> removeToken);
    ImageIoJob addSubscriber(ImageDocumentPageCandidatesCallback callback,
        ErrorCallback errorCallback, QObject *receiver, std::function<void(QObject *)> removeToken);
    void removePendingLoad(QObject *token);
    void removeSubscriber(QObject *token);

private:
    void connectSignals(QObject *signalContext);

    QUrl m_directoryUrl;
    std::unique_ptr<KCoreDirLister> m_lister;
    ImageDocumentPageCandidateStoreEntryState m_state;
};
}

#endif
