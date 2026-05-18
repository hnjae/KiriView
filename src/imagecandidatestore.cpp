// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatestore.h"

#include "imagecallback.h"
#include "imagecandidatedirectoryentry.h"
#include "imageiojobs.h"
#include "imageurl.h"

#include <QPointer>
#include <functional>
#include <memory>
#include <utility>

namespace {
QUrl normalizedDirectoryUrl(QUrl url) { return KiriView::normalizedUrlForIdentity(url); }

QString keyForDirectoryUrl(const QUrl &url)
{
    return KiriView::normalizedUrlIdentityKey(normalizedDirectoryUrl(url), QUrl::FullyEncoded);
}

bool isLiveLocalDirectoryUrl(const QUrl &url)
{
    return url.isLocalFile() && url.isValid() && !url.isEmpty();
}
}

namespace KiriView {
ImageCandidateStore::ImageCandidateStore(QObject *parent)
    : QObject(parent)
{
}

ImageCandidateStore::~ImageCandidateStore() = default;

ImageIoJob ImageCandidateStore::loadDirectoryImages(QObject *receiver, QUrl directoryUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    if (!isLiveLocalDirectoryUrl(directoryUrl)) {
        return startDirectoryImageCandidateList(
            receiver, std::move(directoryUrl), std::move(callback), std::move(errorCallback));
    }

    directoryUrl = normalizedDirectoryUrl(std::move(directoryUrl));
    const QString key = keyForDirectoryUrl(directoryUrl);
    ImageCandidateDirectoryEntry &entry = entryForLocalDirectory(directoryUrl);
    if (entry.failed()) {
        invokeIfSet(errorCallback, entry.errorString());
        return ImageIoJob();
    }
    if (entry.listed()) {
        invokeIfSet(callback, entry.candidates());
        return ImageIoJob();
    }

    QPointer<ImageCandidateStore> store(this);
    return entry.addPendingLoad(
        std::move(callback), std::move(errorCallback), receiver, [store, key](QObject *token) {
            if (!store.isNull()) {
                store->removePendingLoad(key, token);
            }
        });
}

ImageIoJob ImageCandidateStore::watchDirectoryImages(QObject *receiver, QUrl directoryUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    if (!isLiveLocalDirectoryUrl(directoryUrl)) {
        return ImageIoJob();
    }

    directoryUrl = normalizedDirectoryUrl(std::move(directoryUrl));
    const QString key = keyForDirectoryUrl(directoryUrl);
    ImageCandidateDirectoryEntry &entry = entryForLocalDirectory(directoryUrl);
    if (entry.failed()) {
        invokeIfSet(errorCallback, entry.errorString());
        return ImageIoJob();
    }

    QPointer<ImageCandidateStore> store(this);
    return entry.addSubscriber(
        std::move(callback), std::move(errorCallback), receiver, [store, key](QObject *token) {
            if (!store.isNull()) {
                store->removeSubscriber(key, token);
            }
        });
}

ImageCandidateDirectoryEntry &ImageCandidateStore::entryForLocalDirectory(const QUrl &directoryUrl)
{
    const QString key = keyForDirectoryUrl(directoryUrl);
    auto entry = m_entries.find(key);
    if (entry != m_entries.end()) {
        return *entry->second;
    }

    auto insertedEntry = std::make_unique<ImageCandidateDirectoryEntry>(directoryUrl, key, this,
        ImageCandidateDirectoryEntry::Callbacks {
            [this](const QString &entryKey) { handleEntryCompleted(entryKey); },
            [this](const QString &entryKey) { handleEntryChanged(entryKey); },
            [this](const QString &entryKey, const QString &errorString) {
                handleEntryError(entryKey, errorString);
            },
        });
    ImageCandidateDirectoryEntry &entryRef = *insertedEntry;
    m_entries.emplace(key, std::move(insertedEntry));

    if (!entryRef.open()) {
        handleEntryError(key, QString());
    }

    return entryRef;
}

void ImageCandidateStore::handleEntryCompleted(const QString &key)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    entry->second->handleCompleted();
}

void ImageCandidateStore::handleEntryChanged(const QString &key)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    entry->second->handleChanged();
}

void ImageCandidateStore::handleEntryError(const QString &key, const QString &errorString)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    entry->second->handleError(errorString);
}

void ImageCandidateStore::removePendingLoad(const QString &key, QObject *token)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    entry->second->removePendingLoad(token);
}

void ImageCandidateStore::removeSubscriber(const QString &key, QObject *token)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    entry->second->removeSubscriber(token);
}
}
