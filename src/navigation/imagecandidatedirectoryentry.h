// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATEDIRECTORYENTRY_H
#define KIRIVIEW_IMAGECANDIDATEDIRECTORYENTRY_H

#include "imageasynccallbacks.h"
#include "imagecandidatestoreentrystate.h"
#include "imagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <vector>

class KCoreDirLister;
class QObject;

namespace KiriView {
class ImageCandidateDirectoryEntry final
{
public:
    ImageCandidateDirectoryEntry(QUrl directoryUrl, QObject *signalContext);
    ~ImageCandidateDirectoryEntry();

    bool failed() const;
    bool listed() const;
    const QString &errorString() const;
    const std::vector<ImageNavigationCandidate> &candidates() const;

    bool open();
    void handleCompleted();
    void handleChanged();
    void handleError(const QString &errorString);

    ImageIoJob addPendingLoad(ImageCandidatesCallback callback, ErrorCallback errorCallback,
        QObject *receiver, std::function<void(QObject *)> removeToken);
    ImageIoJob addSubscriber(ImageCandidatesCallback callback, ErrorCallback errorCallback,
        QObject *receiver, std::function<void(QObject *)> removeToken);
    void removePendingLoad(QObject *token);
    void removeSubscriber(QObject *token);

private:
    void connectSignals(QObject *signalContext);

    QUrl m_directoryUrl;
    std::unique_ptr<KCoreDirLister> m_lister;
    ImageCandidateStoreEntryState m_state;
};
}

#endif
