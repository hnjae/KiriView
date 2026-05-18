// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatestore.h"

#include "imagecallback.h"
#include "imagecandidatestoreentrystate.h"
#include "imageiojobs.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <KJob>
#include <QPointer>
#include <QTimer>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace {
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

    bool failed() const { return m_state.failed(); }
    bool listed() const { return m_state.listed(); }
    const QString &errorString() const { return m_state.errorString(); }
    const std::vector<ImageNavigationCandidate> &candidates() const { return m_state.candidates(); }
    bool open() { return m_lister->openUrl(m_directoryUrl, KCoreDirLister::Reload); }

    void handleCompleted()
    {
        m_state.completeListing(imageCandidatesForLister(m_lister.get(), m_directoryUrl));
    }

    void handleChanged()
    {
        m_state.updateListing(imageCandidatesForLister(m_lister.get(), m_directoryUrl));
    }

    void handleError(const QString &errorString) { m_state.failListing(errorString); }

    ImageIoJob addPendingLoad(ImageCandidatesCallback callback, ErrorCallback errorCallback,
        QObject *receiver, std::function<void(QObject *)> removeToken)
    {
        return m_state.addPendingLoad(receiver, m_lister.get(), std::move(callback),
            std::move(errorCallback), std::move(removeToken));
    }

    ImageIoJob addSubscriber(ImageCandidatesCallback callback, ErrorCallback errorCallback,
        QObject *receiver, std::function<void(QObject *)> removeToken)
    {
        return m_state.addSubscriber(receiver, m_lister.get(), std::move(callback),
            std::move(errorCallback), std::move(removeToken));
    }

    void removePendingLoad(QObject *token) { m_state.removePendingLoad(token); }

    void removeSubscriber(QObject *token) { m_state.removeSubscriber(token); }

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

    QUrl m_directoryUrl;
    QString m_key;
    std::unique_ptr<KCoreDirLister> m_lister;
    ImageCandidateStoreEntryState m_state;
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
