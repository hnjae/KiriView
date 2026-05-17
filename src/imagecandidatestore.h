// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATESTORE_H
#define KIRIVIEW_IMAGECANDIDATESTORE_H

#include "imageasyncdependencies.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <map>
#include <memory>

namespace KiriView {
class ImageCandidateStore final : public QObject
{
public:
    explicit ImageCandidateStore(QObject *parent = nullptr);
    ~ImageCandidateStore() override;

    ImageIoJob loadDirectoryImages(QObject *receiver, QUrl directoryUrl,
        ImageCandidatesCallback callback, ErrorCallback errorCallback);
    ImageIoJob watchDirectoryImages(QObject *receiver, QUrl directoryUrl,
        ImageCandidatesCallback callback, ErrorCallback errorCallback);

private:
    struct Entry;

    Entry &entryForLocalDirectory(const QUrl &directoryUrl);
    void handleEntryCompleted(const QString &key);
    void handleEntryChanged(const QString &key);
    void handleEntryError(const QString &key, const QString &errorString);
    void removePendingLoad(const QString &key, QObject *token);
    void removeSubscriber(const QString &key, QObject *token);

    std::map<QString, std::unique_ptr<Entry>> m_entries;
};
}

#endif
