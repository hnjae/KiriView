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
class ImageCandidateDirectoryEntry;

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
    ImageCandidateDirectoryEntry &entryForLocalDirectory(const QUrl &directoryUrl);
    void removePendingLoad(const QString &key, QObject *token);
    void removeSubscriber(const QString &key, QObject *token);

    std::map<QString, std::unique_ptr<ImageCandidateDirectoryEntry>> m_entries;
};
}

#endif
