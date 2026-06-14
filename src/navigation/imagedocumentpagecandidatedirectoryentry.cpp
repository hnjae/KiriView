// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagecandidatedirectoryentry.h"

#include "async/imagecallback.h"
#include "location/imageurl.h"

#include <algorithm>
#include <utility>

namespace {
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
    QUrl directoryUrl, ImageDocumentPageCandidateWatchProvider watchProvider,
    QObject *signalContext)
    : m_directoryUrl(std::move(directoryUrl))
    , m_watchProvider(std::move(watchProvider))
    , m_signalContext(signalContext)
{
    if (!m_watchProvider) {
        m_watchProvider = defaultImageDocumentPageCandidateWatchProvider();
    }
}

ImageDocumentPageCandidateDirectoryEntry::~ImageDocumentPageCandidateDirectoryEntry()
{
    m_watchJob.cancel();
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
    if (m_watchJob.isActive() || !m_watchProvider) {
        return m_watchJob.isActive();
    }

    m_watchJob = m_watchProvider(
        m_signalContext, m_directoryUrl,
        [this](std::vector<ImageDocumentPageCandidate> candidates) {
            handleCompleted(std::move(candidates));
        },
        [this](std::vector<ImageDocumentPageCandidate> candidates) {
            handleChanged(std::move(candidates));
        },
        [this](QList<QUrl> urls) { handleDeleted(std::move(urls)); },
        [this](const QString &errorString) { handleError(errorString); });
    return m_watchJob.isActive();
}

void ImageDocumentPageCandidateDirectoryEntry::handleCompleted(
    std::vector<ImageDocumentPageCandidate> candidates)
{
    applyEntryNotificationPlan(m_state.completeListing(std::move(candidates)));
}

void ImageDocumentPageCandidateDirectoryEntry::handleChanged(
    std::vector<ImageDocumentPageCandidate> candidates)
{
    applyEntryNotificationPlan(m_state.updateListing(std::move(candidates)));
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
    QObject *token = createEntryJobToken(receiver, m_signalContext);
    ImageIoJob job = createEntryJob(token, std::move(removeToken));
    m_state.addPendingLoad(job.completion(), std::move(callback), std::move(errorCallback));
    return job;
}

ImageIoJob ImageDocumentPageCandidateDirectoryEntry::addSubscriber(
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback, QObject *receiver,
    std::function<void(QObject *)> removeToken)
{
    QObject *token = createEntryJobToken(receiver, m_signalContext);
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

}
