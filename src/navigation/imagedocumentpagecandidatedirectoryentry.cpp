// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagecandidatedirectoryentry.h"

#include "async/imagecallback.h"
#include "imagedocumentpagecandidateitems.h"
#include "location/imageurl.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <QTimer>
#include <algorithm>
#include <memory>
#include <utility>

namespace {
KCoreDirLister *createLiveImageDocumentPageCandidateLister()
{
    auto *lister = new KCoreDirLister();
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(true);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    return lister;
}

std::vector<kiriview::ImageDocumentPageCandidate> imageDocumentPageCandidatesForLister(
    KCoreDirLister *lister, const QUrl &directoryUrl)
{
    return kiriview::imageDocumentPageNavigationCandidates(
        lister->itemsForDir(directoryUrl, KCoreDirLister::AllItems));
}

QList<QUrl> urlsForItems(const KFileItemList &items)
{
    QList<QUrl> urls;
    urls.reserve(items.size());
    for (const KFileItem &item : items) {
        urls.push_back(item.url());
    }
    return urls;
}

std::vector<kiriview::ImageDocumentPageCandidate> imageDocumentPageCandidatesWithoutDeletedUrls(
    const std::vector<kiriview::ImageDocumentPageCandidate> &candidates,
    const QList<QUrl> &deletedUrls)
{
    std::vector<kiriview::ImageDocumentPageCandidate> filteredCandidates;
    filteredCandidates.reserve(candidates.size());
    for (const kiriview::ImageDocumentPageCandidate &candidate : candidates) {
        const bool removed
            = std::any_of(deletedUrls.cbegin(), deletedUrls.cend(), [&candidate](const QUrl &url) {
                  return kiriview::sameNormalizedUrl(candidate.url, url);
              });
        if (!removed) {
            filteredCandidates.push_back(candidate);
        }
    }
    return filteredCandidates;
}

QObject *createEntryJobToken(QObject *receiver, QObject *fallbackParent)
{
    return new QObject(receiver == nullptr ? fallbackParent : receiver);
}

kiriview::ImageIoJob createEntryJob(QObject *token, std::function<void(QObject *)> removeToken)
{
    return kiriview::ImageIoJob(token, [removeToken = std::move(removeToken)](QObject *object) {
        removeToken(object);
        object->deleteLater();
    });
}

void notifyChanged(QObject *context, kiriview::ImageDocumentPageCandidateDirectoryEntry *entry)
{
    QTimer::singleShot(0, context, [entry]() {
        if (entry != nullptr) {
            entry->handleChanged();
        }
    });
}

void applyEntryNotificationPlan(kiriview::ImageDocumentPageCandidateStoreEntryNotificationPlan plan)
{
    for (const kiriview::ImageDocumentPageCandidateStoreEntryPendingLoad &load :
        plan.completedLoads) {
        load.completion.claimAndDelete(
            [&]() { kiriview::invokeIfSet(load.callback, plan.candidates); });
    }
    for (const kiriview::ImageDocumentPageCandidateStoreEntrySubscriber &subscriber :
        plan.changedSubscribers) {
        kiriview::invokeIfSet(subscriber.callback, plan.candidates);
    }
    for (const kiriview::ImageDocumentPageCandidateStoreEntryPendingLoad &load : plan.failedLoads) {
        load.completion.claimAndDelete(
            [&]() { kiriview::invokeIfSet(load.errorCallback, plan.errorString); });
    }
    for (const kiriview::ImageDocumentPageCandidateStoreEntrySubscriber &subscriber :
        plan.failedSubscribers) {
        kiriview::invokeIfSet(subscriber.errorCallback, plan.errorString);
    }
}
}

namespace kiriview {
ImageDocumentPageCandidateDirectoryEntry::ImageDocumentPageCandidateDirectoryEntry(
    QUrl directoryUrl, QObject *signalContext)
    : m_directoryUrl(std::move(directoryUrl))
    , m_lister(createLiveImageDocumentPageCandidateLister())
{
    connectSignals(signalContext);
}

ImageDocumentPageCandidateDirectoryEntry::~ImageDocumentPageCandidateDirectoryEntry()
{
    QObject::disconnect(m_lister.get(), nullptr, nullptr, nullptr);
    m_lister->stop();
}

bool ImageDocumentPageCandidateDirectoryEntry::failed() const { return m_state.failed(); }

bool ImageDocumentPageCandidateDirectoryEntry::listed() const { return m_state.listed(); }

const QString &ImageDocumentPageCandidateDirectoryEntry::errorString() const
{
    return m_state.errorString();
}

const std::vector<ImageDocumentPageCandidate> &
ImageDocumentPageCandidateDirectoryEntry::candidates() const
{
    return m_state.candidates();
}

bool ImageDocumentPageCandidateDirectoryEntry::open()
{
    return m_lister->openUrl(m_directoryUrl, KCoreDirLister::Reload);
}

void ImageDocumentPageCandidateDirectoryEntry::handleCompleted()
{
    applyEntryNotificationPlan(m_state.completeListing(
        imageDocumentPageCandidatesForLister(m_lister.get(), m_directoryUrl)));
}

void ImageDocumentPageCandidateDirectoryEntry::handleChanged()
{
    applyEntryNotificationPlan(m_state.updateListing(
        imageDocumentPageCandidatesForLister(m_lister.get(), m_directoryUrl)));
}

void ImageDocumentPageCandidateDirectoryEntry::handleDeleted(const KFileItemList &items)
{
    handleDeleted(urlsForItems(items));
}

void ImageDocumentPageCandidateDirectoryEntry::handleDeleted(const QList<QUrl> &urls)
{
    applyEntryNotificationPlan(
        m_state.updateListing(imageDocumentPageCandidatesWithoutDeletedUrls(candidates(), urls)));
}

void ImageDocumentPageCandidateDirectoryEntry::handleError(const QString &errorString)
{
    applyEntryNotificationPlan(m_state.failListing(errorString));
}

ImageIoJob ImageDocumentPageCandidateDirectoryEntry::addPendingLoad(
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback, QObject *receiver,
    std::function<void(QObject *)> removeToken)
{
    QObject *token = createEntryJobToken(receiver, m_lister.get());
    ImageIoJob job = createEntryJob(token, std::move(removeToken));
    m_state.addPendingLoad(job.completion(), std::move(callback), std::move(errorCallback));
    return job;
}

ImageIoJob ImageDocumentPageCandidateDirectoryEntry::addSubscriber(
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback, QObject *receiver,
    std::function<void(QObject *)> removeToken)
{
    QObject *token = createEntryJobToken(receiver, m_lister.get());
    ImageIoJob job = createEntryJob(token, std::move(removeToken));
    m_state.addSubscriber(token, std::move(callback), std::move(errorCallback));
    return job;
}

void ImageDocumentPageCandidateDirectoryEntry::removePendingLoad(QObject *token)
{
    m_state.removePendingLoad(token);
}

void ImageDocumentPageCandidateDirectoryEntry::removeSubscriber(QObject *token)
{
    m_state.removeSubscriber(token);
}

void ImageDocumentPageCandidateDirectoryEntry::connectSignals(QObject *signalContext)
{
    QObject *context = signalContext == nullptr ? m_lister.get() : signalContext;

    QObject::connect(
        m_lister.get(), &KCoreDirLister::completed, context, [this]() { handleCompleted(); });
    QObject::connect(m_lister.get(), &KCoreDirLister::itemsAdded, context,
        [this, context](const QUrl &, const KFileItemList &) { notifyChanged(context, this); });
    QObject::connect(m_lister.get(), &KCoreDirLister::itemsDeleted, context,
        [this](const KFileItemList &items) { handleDeleted(items); });
    QObject::connect(m_lister.get(), &KCoreDirLister::refreshItems, context,
        [this, context](
            const QList<QPair<KFileItem, KFileItem>> &) { notifyChanged(context, this); });
    QObject::connect(m_lister.get(), &KCoreDirLister::clear, context,
        [this, context]() { notifyChanged(context, this); });
    QObject::connect(m_lister.get(), &KCoreDirLister::clearDir, context,
        [this, context](const QUrl &) { notifyChanged(context, this); });
    QObject::connect(m_lister.get(), &KCoreDirLister::jobError, context,
        [this](KIO::Job *job) { handleError(job == nullptr ? QString() : job->errorString()); });
}
}
