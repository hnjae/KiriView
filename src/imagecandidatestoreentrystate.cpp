// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatestoreentrystate.h"

#include "imagecallback.h"
#include "imageurl.h"

#include <algorithm>
#include <utility>

namespace {
QObject *createJobToken(QObject *receiver, QObject *fallbackParent)
{
    return new QObject(receiver == nullptr ? fallbackParent : receiver);
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

template <typename Item> void pruneInactiveItems(std::vector<Item> *items)
{
    const auto inactiveStart = std::remove_if(
        items->begin(), items->end(), [](const Item &item) { return item.token.isNull(); });
    items->erase(inactiveStart, items->end());
}
}

namespace KiriView {
const std::vector<ImageNavigationCandidate> &ImageCandidateStoreEntryState::candidates() const
{
    return m_candidates;
}

bool ImageCandidateStoreEntryState::listed() const { return m_listed; }

bool ImageCandidateStoreEntryState::failed() const { return m_failed; }

const QString &ImageCandidateStoreEntryState::errorString() const { return m_errorString; }

ImageIoJob ImageCandidateStoreEntryState::addPendingLoad(QObject *receiver, QObject *fallbackParent,
    ImageCandidatesCallback callback, ErrorCallback errorCallback,
    std::function<void(QObject *)> removeToken)
{
    QObject *token = createJobToken(receiver, fallbackParent);
    ImageIoJob job(token, [removeToken = std::move(removeToken)](QObject *object) {
        removeToken(object);
        object->deleteLater();
    });
    m_pendingLoads.push_back(PendingLoad {
        job.completion(),
        std::move(callback),
        std::move(errorCallback),
    });
    return job;
}

ImageIoJob ImageCandidateStoreEntryState::addSubscriber(QObject *receiver, QObject *fallbackParent,
    ImageCandidatesCallback callback, ErrorCallback errorCallback,
    std::function<void(QObject *)> removeToken)
{
    QObject *token = createJobToken(receiver, fallbackParent);
    ImageIoJob job(token, [removeToken = std::move(removeToken)](QObject *object) {
        removeToken(object);
        object->deleteLater();
    });
    m_subscribers.push_back(Subscriber {
        token,
        std::move(callback),
        std::move(errorCallback),
    });
    return job;
}

void ImageCandidateStoreEntryState::removePendingLoad(QObject *token)
{
    const auto removed = std::remove_if(m_pendingLoads.begin(), m_pendingLoads.end(),
        [token](const PendingLoad &load) { return load.completion.object() == token; });
    m_pendingLoads.erase(removed, m_pendingLoads.end());
}

void ImageCandidateStoreEntryState::removeSubscriber(QObject *token)
{
    const auto removed = std::remove_if(m_subscribers.begin(), m_subscribers.end(),
        [token](const Subscriber &subscriber) { return subscriber.token.data() == token; });
    m_subscribers.erase(removed, m_subscribers.end());
}

void ImageCandidateStoreEntryState::completeListing(
    std::vector<ImageNavigationCandidate> candidates)
{
    const bool wasListed = m_listed;
    const bool changed = replaceCandidates(std::move(candidates));
    m_listed = true;
    m_failed = false;
    m_errorString = QString();
    finishPendingLoads();

    if (wasListed && changed) {
        notifySubscribers();
    }
}

void ImageCandidateStoreEntryState::updateListing(std::vector<ImageNavigationCandidate> candidates)
{
    const bool changed = replaceCandidates(std::move(candidates));
    if (m_listed && changed) {
        notifySubscribers();
    }
}

void ImageCandidateStoreEntryState::failListing(QString errorString)
{
    m_failed = true;
    m_errorString = std::move(errorString);
    finishPendingLoadErrors();
    notifySubscriberErrors();
}

bool ImageCandidateStoreEntryState::replaceCandidates(
    std::vector<ImageNavigationCandidate> candidates)
{
    if (sameImageNavigationCandidates(m_candidates, candidates)) {
        return false;
    }

    m_candidates = std::move(candidates);
    return true;
}

void ImageCandidateStoreEntryState::finishPendingLoads()
{
    std::vector<PendingLoad> pendingLoads = std::move(m_pendingLoads);
    m_pendingLoads.clear();
    for (PendingLoad &load : pendingLoads) {
        load.completion.claimAndDelete([&]() { invokeIfSet(load.callback, m_candidates); });
    }
}

void ImageCandidateStoreEntryState::finishPendingLoadErrors()
{
    std::vector<PendingLoad> pendingLoads = std::move(m_pendingLoads);
    m_pendingLoads.clear();
    for (PendingLoad &load : pendingLoads) {
        load.completion.claimAndDelete([&]() { invokeIfSet(load.errorCallback, m_errorString); });
    }
}

void ImageCandidateStoreEntryState::notifySubscribers()
{
    pruneInactiveItems(&m_subscribers);
    const std::vector<Subscriber> subscribers = m_subscribers;
    const std::vector<ImageNavigationCandidate> candidates = m_candidates;
    for (const Subscriber &subscriber : subscribers) {
        invokeIfSet(subscriber.callback, candidates);
    }
}

void ImageCandidateStoreEntryState::notifySubscriberErrors()
{
    pruneInactiveItems(&m_subscribers);
    const std::vector<Subscriber> subscribers = m_subscribers;
    const QString errorString = m_errorString;
    for (const Subscriber &subscriber : subscribers) {
        invokeIfSet(subscriber.errorCallback, errorString);
    }
}
}
