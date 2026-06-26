// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATESTORE_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATESTORE_H

#include "imagedocumentpagecandidateprovider.h"
#include "imagedocumentpagecandidatewatchprovider.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <map>
#include <memory>

namespace kiriview {
class ImageDocumentPageCandidateDirectoryEntry;

class ImageDocumentPageCandidateStore final : public QObject
{
public:
    explicit ImageDocumentPageCandidateStore(QObject* parent = nullptr);
    explicit ImageDocumentPageCandidateStore(
        ImageDocumentPageCandidateWatchProvider watchProvider, QObject* parent = nullptr);
    ~ImageDocumentPageCandidateStore() override;

    ImageIoJob loadDirectoryImages(QObject* receiver, QUrl directoryUrl,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback);
    ImageIoJob watchDirectoryImages(QObject* receiver, QUrl directoryUrl,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback);

private:
    ImageDocumentPageCandidateDirectoryEntry& entryForLocalDirectory(const QUrl& directoryUrl);
    void removePendingLoad(const QString& key, QObject* token);
    void removeSubscriber(const QString& key, QObject* token);

    std::map<QString, std::unique_ptr<ImageDocumentPageCandidateDirectoryEntry>> m_entries;
    ImageDocumentPageCandidateWatchProvider m_watchProvider;
};
}

#endif
