// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatestore.h"

#include "imagecallback.h"
#include "imageiojobs.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <KJob>
#include <QPointer>
#include <QTimer>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace {
struct PendingImageCandidateLoad {
    QPointer<QObject> token;
    std::shared_ptr<KiriView::ImageIoJobState> state;
    KiriView::ImageCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
};

struct ImageCandidateSubscriber {
    QPointer<QObject> token;
    KiriView::ImageCandidatesCallback callback;
    KiriView::ErrorCallback errorCallback;
};

KCoreDirLister *createLiveImageCandidateLister()
{
    auto *lister = new KCoreDirLister();
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(true);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    return lister;
}

QUrl normalizedDirectoryUrl(QUrl url) { return url.adjusted(QUrl::NormalizePathSegments); }

QString keyForDirectoryUrl(const QUrl &url)
{
    return normalizedDirectoryUrl(url).toString(QUrl::FullyEncoded);
}

bool isLiveLocalDirectoryUrl(const QUrl &url)
{
    return url.isLocalFile() && url.isValid() && !url.isEmpty();
}

std::vector<KiriView::ImageNavigationCandidate> imageCandidatesForLister(
    KCoreDirLister *lister, const QUrl &directoryUrl)
{
    return KiriView::imageNavigationCandidates(
        lister->itemsForDir(directoryUrl, KCoreDirLister::AllItems));
}

bool sameImageNavigationCandidates(const std::vector<KiriView::ImageNavigationCandidate> &left,
    const std::vector<KiriView::ImageNavigationCandidate> &right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.size(); ++index) {
        if (!KiriView::sameNormalizedUrl(left[index].url, right[index].url)
            || left[index].name != right[index].name) {
            return false;
        }
    }

    return true;
}

QObject *createJobToken(QObject *receiver, QObject *fallbackParent)
{
    return new QObject(receiver == nullptr ? fallbackParent : receiver);
}

template <typename Item> void pruneInactiveItems(std::vector<Item> *items)
{
    const auto inactiveStart = std::remove_if(
        items->begin(), items->end(), [](const Item &item) { return item.token.isNull(); });
    items->erase(inactiveStart, items->end());
}
}

namespace KiriView {
struct ImageCandidateStore::Entry {
    explicit Entry(QUrl directoryUrl)
        : directoryUrl(std::move(directoryUrl))
        , lister(createLiveImageCandidateLister())
    {
    }

    ~Entry()
    {
        QObject::disconnect(lister.get(), nullptr, nullptr, nullptr);
        lister->stop();
    }

    QUrl directoryUrl;
    std::unique_ptr<KCoreDirLister> lister;
    std::vector<ImageNavigationCandidate> candidates;
    std::vector<PendingImageCandidateLoad> pendingLoads;
    std::vector<ImageCandidateSubscriber> subscribers;
    bool listed = false;
    bool failed = false;
    QString errorString;
};

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
    Entry &entry = entryForLocalDirectory(directoryUrl);
    if (entry.failed) {
        invokeIfSet(errorCallback, entry.errorString);
        return ImageIoJob();
    }
    if (entry.listed) {
        invokeIfSet(callback, entry.candidates);
        return ImageIoJob();
    }

    return addPendingLoad(key, entry, std::move(callback), std::move(errorCallback), receiver);
}

ImageIoJob ImageCandidateStore::watchDirectoryImages(QObject *receiver, QUrl directoryUrl,
    ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    if (!isLiveLocalDirectoryUrl(directoryUrl)) {
        return ImageIoJob();
    }

    directoryUrl = normalizedDirectoryUrl(std::move(directoryUrl));
    const QString key = keyForDirectoryUrl(directoryUrl);
    Entry &entry = entryForLocalDirectory(directoryUrl);
    if (entry.failed) {
        invokeIfSet(errorCallback, entry.errorString);
        return ImageIoJob();
    }

    return addSubscriber(key, entry, std::move(callback), std::move(errorCallback), receiver);
}

ImageCandidateStore::Entry &ImageCandidateStore::entryForLocalDirectory(const QUrl &directoryUrl)
{
    const QString key = keyForDirectoryUrl(directoryUrl);
    auto entry = m_entries.find(key);
    if (entry != m_entries.end()) {
        return *entry->second;
    }

    auto insertedEntry = std::make_unique<Entry>(directoryUrl);
    Entry &entryRef = *insertedEntry;
    connectEntrySignals(key, entryRef);
    m_entries.emplace(key, std::move(insertedEntry));

    if (!entryRef.lister->openUrl(directoryUrl, KCoreDirLister::Reload)) {
        handleEntryError(key, QString());
    }

    return entryRef;
}

void ImageCandidateStore::connectEntrySignals(const QString &key, Entry &entry)
{
    QObject::connect(entry.lister.get(), &KCoreDirLister::completed, this,
        [this, key]() { handleEntryCompleted(key); });
    QObject::connect(entry.lister.get(), &KCoreDirLister::itemsAdded, this,
        [this, key](const QUrl &, const KFileItemList &) {
            QTimer::singleShot(0, this, [this, key]() { handleEntryChanged(key); });
        });
    QObject::connect(entry.lister.get(), &KCoreDirLister::itemsDeleted, this,
        [this, key](const KFileItemList &) {
            QTimer::singleShot(0, this, [this, key]() { handleEntryChanged(key); });
        });
    QObject::connect(entry.lister.get(), &KCoreDirLister::refreshItems, this,
        [this, key](const QList<QPair<KFileItem, KFileItem>> &) {
            QTimer::singleShot(0, this, [this, key]() { handleEntryChanged(key); });
        });
    QObject::connect(entry.lister.get(), &KCoreDirLister::clear, this,
        [this, key]() { QTimer::singleShot(0, this, [this, key]() { handleEntryChanged(key); }); });
    QObject::connect(
        entry.lister.get(), &KCoreDirLister::clearDir, this, [this, key](const QUrl &) {
            QTimer::singleShot(0, this, [this, key]() { handleEntryChanged(key); });
        });
    QObject::connect(
        entry.lister.get(), &KCoreDirLister::jobError, this, [this, key](KIO::Job *job) {
            handleEntryError(key, job == nullptr ? QString() : job->errorString());
        });
}

void ImageCandidateStore::handleEntryCompleted(const QString &key)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    const bool wasListed = entry->second->listed;
    const bool changed = updateEntryCandidates(*entry->second);
    entry->second->listed = true;
    entry->second->failed = false;
    entry->second->errorString = QString();
    finishPendingLoads(*entry->second);

    if (wasListed && changed) {
        notifySubscribers(*entry->second);
    }
}

void ImageCandidateStore::handleEntryChanged(const QString &key)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    const bool changed = updateEntryCandidates(*entry->second);
    if (entry->second->listed && changed) {
        notifySubscribers(*entry->second);
    }
}

void ImageCandidateStore::handleEntryError(const QString &key, const QString &errorString)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    entry->second->failed = true;
    entry->second->errorString = errorString;
    finishPendingLoadErrors(*entry->second);
    notifySubscriberErrors(*entry->second);
}

bool ImageCandidateStore::updateEntryCandidates(Entry &entry)
{
    std::vector<ImageNavigationCandidate> candidates
        = imageCandidatesForLister(entry.lister.get(), entry.directoryUrl);
    if (sameImageNavigationCandidates(entry.candidates, candidates)) {
        return false;
    }

    entry.candidates = std::move(candidates);
    return true;
}

void ImageCandidateStore::finishPendingLoads(Entry &entry)
{
    std::vector<PendingImageCandidateLoad> pendingLoads = std::move(entry.pendingLoads);
    entry.pendingLoads.clear();
    for (PendingImageCandidateLoad &load : pendingLoads) {
        QObject *token = load.token.data();
        if (load.state == nullptr || token == nullptr) {
            continue;
        }

        load.state->claimAndRun(token, [&]() {
            invokeIfSet(load.callback, entry.candidates);
            token->deleteLater();
        });
    }
}

void ImageCandidateStore::finishPendingLoadErrors(Entry &entry)
{
    std::vector<PendingImageCandidateLoad> pendingLoads = std::move(entry.pendingLoads);
    entry.pendingLoads.clear();
    for (PendingImageCandidateLoad &load : pendingLoads) {
        QObject *token = load.token.data();
        if (load.state == nullptr || token == nullptr) {
            continue;
        }

        load.state->claimAndRun(token, [&]() {
            invokeIfSet(load.errorCallback, entry.errorString);
            token->deleteLater();
        });
    }
}

void ImageCandidateStore::notifySubscribers(Entry &entry)
{
    pruneInactiveItems(&entry.subscribers);
    const std::vector<ImageCandidateSubscriber> subscribers = entry.subscribers;
    const std::vector<ImageNavigationCandidate> candidates = entry.candidates;
    for (const ImageCandidateSubscriber &subscriber : subscribers) {
        invokeIfSet(subscriber.callback, candidates);
    }
}

void ImageCandidateStore::notifySubscriberErrors(Entry &entry)
{
    pruneInactiveItems(&entry.subscribers);
    const std::vector<ImageCandidateSubscriber> subscribers = entry.subscribers;
    const QString errorString = entry.errorString;
    for (const ImageCandidateSubscriber &subscriber : subscribers) {
        invokeIfSet(subscriber.errorCallback, errorString);
    }
}

ImageIoJob ImageCandidateStore::addPendingLoad(const QString &key, Entry &entry,
    ImageCandidatesCallback callback, ErrorCallback errorCallback, QObject *receiver)
{
    QObject *token = createJobToken(receiver, this);
    ImageIoJob job(token, [this, key](QObject *object) {
        removePendingLoad(key, object);
        object->deleteLater();
    });
    entry.pendingLoads.push_back(PendingImageCandidateLoad {
        token,
        job.state(),
        std::move(callback),
        std::move(errorCallback),
    });
    return job;
}

ImageIoJob ImageCandidateStore::addSubscriber(const QString &key, Entry &entry,
    ImageCandidatesCallback callback, ErrorCallback errorCallback, QObject *receiver)
{
    QObject *token = createJobToken(receiver, this);
    ImageIoJob job(token, [this, key](QObject *object) {
        removeSubscriber(key, object);
        object->deleteLater();
    });
    entry.subscribers.push_back(ImageCandidateSubscriber {
        token,
        std::move(callback),
        std::move(errorCallback),
    });
    return job;
}

void ImageCandidateStore::removePendingLoad(const QString &key, QObject *token)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    auto &pendingLoads = entry->second->pendingLoads;
    const auto removed = std::remove_if(pendingLoads.begin(), pendingLoads.end(),
        [token](const PendingImageCandidateLoad &load) { return load.token.data() == token; });
    pendingLoads.erase(removed, pendingLoads.end());
}

void ImageCandidateStore::removeSubscriber(const QString &key, QObject *token)
{
    auto entry = m_entries.find(key);
    if (entry == m_entries.end()) {
        return;
    }

    auto &subscribers = entry->second->subscribers;
    const auto removed = std::remove_if(subscribers.begin(), subscribers.end(),
        [token](const ImageCandidateSubscriber &subscriber) {
            return subscriber.token.data() == token;
        });
    subscribers.erase(removed, subscribers.end());
}
}
