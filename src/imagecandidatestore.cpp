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
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace {
struct PendingImageCandidateLoad {
    KiriView::ImageIoJobCompletion completion;
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

QUrl normalizedDirectoryUrl(QUrl url) { return KiriView::normalizedUrlForIdentity(url); }

QString keyForDirectoryUrl(const QUrl &url)
{
    return KiriView::normalizedUrlIdentityKey(normalizedDirectoryUrl(url), QUrl::FullyEncoded);
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
    Entry(QUrl directoryUrl, ImageCandidateStore &store, QString key)
        : m_directoryUrl(std::move(directoryUrl))
        , m_key(std::move(key))
        , m_lister(createLiveImageCandidateLister())
    {
        connectSignals(store);
    }

    ~Entry()
    {
        QObject::disconnect(m_lister.get(), nullptr, nullptr, nullptr);
        m_lister->stop();
    }

    bool failed() const { return m_failed; }
    bool listed() const { return m_listed; }
    const QString &errorString() const { return m_errorString; }
    const std::vector<ImageNavigationCandidate> &candidates() const { return m_candidates; }
    bool open() { return m_lister->openUrl(m_directoryUrl, KCoreDirLister::Reload); }

    void handleCompleted()
    {
        const bool wasListed = m_listed;
        const bool changed = updateCandidates();
        m_listed = true;
        m_failed = false;
        m_errorString = QString();
        finishPendingLoads();

        if (wasListed && changed) {
            notifySubscribers();
        }
    }

    void handleChanged()
    {
        const bool changed = updateCandidates();
        if (m_listed && changed) {
            notifySubscribers();
        }
    }

    void handleError(const QString &errorString)
    {
        m_failed = true;
        m_errorString = errorString;
        finishPendingLoadErrors();
        notifySubscriberErrors();
    }

    ImageIoJob addPendingLoad(ImageCandidatesCallback callback, ErrorCallback errorCallback,
        QObject *receiver, std::function<void(QObject *)> removeToken)
    {
        QObject *token = createJobToken(receiver, m_lister.get());
        ImageIoJob job(token, [removeToken = std::move(removeToken)](QObject *object) {
            removeToken(object);
            object->deleteLater();
        });
        m_pendingLoads.push_back(PendingImageCandidateLoad {
            job.completion(),
            std::move(callback),
            std::move(errorCallback),
        });
        return job;
    }

    ImageIoJob addSubscriber(ImageCandidatesCallback callback, ErrorCallback errorCallback,
        QObject *receiver, std::function<void(QObject *)> removeToken)
    {
        QObject *token = createJobToken(receiver, m_lister.get());
        ImageIoJob job(token, [removeToken = std::move(removeToken)](QObject *object) {
            removeToken(object);
            object->deleteLater();
        });
        m_subscribers.push_back(ImageCandidateSubscriber {
            token,
            std::move(callback),
            std::move(errorCallback),
        });
        return job;
    }

    void removePendingLoad(QObject *token)
    {
        const auto removed = std::remove_if(m_pendingLoads.begin(), m_pendingLoads.end(),
            [token](const PendingImageCandidateLoad &load) {
                return load.completion.object() == token;
            });
        m_pendingLoads.erase(removed, m_pendingLoads.end());
    }

    void removeSubscriber(QObject *token)
    {
        const auto removed = std::remove_if(m_subscribers.begin(), m_subscribers.end(),
            [token](const ImageCandidateSubscriber &subscriber) {
                return subscriber.token.data() == token;
            });
        m_subscribers.erase(removed, m_subscribers.end());
    }

private:
    void connectSignals(ImageCandidateStore &store)
    {
        QObject::connect(m_lister.get(), &KCoreDirLister::completed, &store,
            [&store, key = m_key]() { store.handleEntryCompleted(key); });
        QObject::connect(m_lister.get(), &KCoreDirLister::itemsAdded, &store,
            [&store, key = m_key](const QUrl &, const KFileItemList &) {
                QTimer::singleShot(0, &store, [&store, key]() { store.handleEntryChanged(key); });
            });
        QObject::connect(m_lister.get(), &KCoreDirLister::itemsDeleted, &store,
            [&store, key = m_key](const KFileItemList &) {
                QTimer::singleShot(0, &store, [&store, key]() { store.handleEntryChanged(key); });
            });
        QObject::connect(m_lister.get(), &KCoreDirLister::refreshItems, &store,
            [&store, key = m_key](const QList<QPair<KFileItem, KFileItem>> &) {
                QTimer::singleShot(0, &store, [&store, key]() { store.handleEntryChanged(key); });
            });
        QObject::connect(m_lister.get(), &KCoreDirLister::clear, &store, [&store, key = m_key]() {
            QTimer::singleShot(0, &store, [&store, key]() { store.handleEntryChanged(key); });
        });
        QObject::connect(
            m_lister.get(), &KCoreDirLister::clearDir, &store, [&store, key = m_key](const QUrl &) {
                QTimer::singleShot(0, &store, [&store, key]() { store.handleEntryChanged(key); });
            });
        QObject::connect(m_lister.get(), &KCoreDirLister::jobError, &store,
            [&store, key = m_key](KIO::Job *job) {
                store.handleEntryError(key, job == nullptr ? QString() : job->errorString());
            });
    }

    bool updateCandidates()
    {
        std::vector<ImageNavigationCandidate> candidates
            = imageCandidatesForLister(m_lister.get(), m_directoryUrl);
        if (sameImageNavigationCandidates(m_candidates, candidates)) {
            return false;
        }

        m_candidates = std::move(candidates);
        return true;
    }

    void finishPendingLoads()
    {
        std::vector<PendingImageCandidateLoad> pendingLoads = std::move(m_pendingLoads);
        m_pendingLoads.clear();
        for (PendingImageCandidateLoad &load : pendingLoads) {
            load.completion.claimAndDelete([&]() { invokeIfSet(load.callback, m_candidates); });
        }
    }

    void finishPendingLoadErrors()
    {
        std::vector<PendingImageCandidateLoad> pendingLoads = std::move(m_pendingLoads);
        m_pendingLoads.clear();
        for (PendingImageCandidateLoad &load : pendingLoads) {
            load.completion.claimAndDelete(
                [&]() { invokeIfSet(load.errorCallback, m_errorString); });
        }
    }

    void notifySubscribers()
    {
        pruneInactiveItems(&m_subscribers);
        const std::vector<ImageCandidateSubscriber> subscribers = m_subscribers;
        const std::vector<ImageNavigationCandidate> candidates = m_candidates;
        for (const ImageCandidateSubscriber &subscriber : subscribers) {
            invokeIfSet(subscriber.callback, candidates);
        }
    }

    void notifySubscriberErrors()
    {
        pruneInactiveItems(&m_subscribers);
        const std::vector<ImageCandidateSubscriber> subscribers = m_subscribers;
        const QString errorString = m_errorString;
        for (const ImageCandidateSubscriber &subscriber : subscribers) {
            invokeIfSet(subscriber.errorCallback, errorString);
        }
    }

    QUrl m_directoryUrl;
    QString m_key;
    std::unique_ptr<KCoreDirLister> m_lister;
    std::vector<ImageNavigationCandidate> m_candidates;
    std::vector<PendingImageCandidateLoad> m_pendingLoads;
    std::vector<ImageCandidateSubscriber> m_subscribers;
    bool m_listed = false;
    bool m_failed = false;
    QString m_errorString;
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
    Entry &entry = entryForLocalDirectory(directoryUrl);
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

ImageCandidateStore::Entry &ImageCandidateStore::entryForLocalDirectory(const QUrl &directoryUrl)
{
    const QString key = keyForDirectoryUrl(directoryUrl);
    auto entry = m_entries.find(key);
    if (entry != m_entries.end()) {
        return *entry->second;
    }

    auto insertedEntry = std::make_unique<Entry>(directoryUrl, *this, key);
    Entry &entryRef = *insertedEntry;
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
