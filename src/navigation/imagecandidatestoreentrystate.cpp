// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatestoreentrystate.h"

#include "location/imageurl.h"

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
    m_pendingLoads.push_back(ImageCandidateStoreEntryPendingLoad {
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
    m_subscribers.push_back(ImageCandidateStoreEntrySubscriber {
        token,
        std::move(callback),
        std::move(errorCallback),
    });
    return job;
}

void ImageCandidateStoreEntryState::removePendingLoad(QObject *token)
{
    const auto removed = std::remove_if(m_pendingLoads.begin(), m_pendingLoads.end(),
        [token](const ImageCandidateStoreEntryPendingLoad &load) {
            return load.completion.object() == token;
        });
    m_pendingLoads.erase(removed, m_pendingLoads.end());
}

void ImageCandidateStoreEntryState::removeSubscriber(QObject *token)
{
    const auto removed = std::remove_if(m_subscribers.begin(), m_subscribers.end(),
        [token](const ImageCandidateStoreEntrySubscriber &subscriber) {
            return subscriber.token.data() == token;
        });
    m_subscribers.erase(removed, m_subscribers.end());
}

ImageCandidateStoreEntryNotificationPlan ImageCandidateStoreEntryState::completeListing(
    std::vector<ImageNavigationCandidate> candidates)
{
    const bool wasListed = m_listed;
    const bool changed = replaceCandidates(std::move(candidates));
    m_listed = true;
    m_failed = false;
    m_errorString = QString();

    ImageCandidateStoreEntryNotificationPlan plan;
    plan.completedLoads = takePendingLoads();
    plan.candidates = m_candidates;
    if (wasListed && changed) {
        plan.changedSubscribers = activeSubscribers();
    }
    return plan;
}

ImageCandidateStoreEntryNotificationPlan ImageCandidateStoreEntryState::updateListing(
    std::vector<ImageNavigationCandidate> candidates)
{
    const bool changed = replaceCandidates(std::move(candidates));
    ImageCandidateStoreEntryNotificationPlan plan;
    plan.candidates = m_candidates;
    if (m_listed && changed) {
        plan.changedSubscribers = activeSubscribers();
    }
    return plan;
}

ImageCandidateStoreEntryNotificationPlan ImageCandidateStoreEntryState::failListing(
    QString errorString)
{
    m_failed = true;
    m_errorString = std::move(errorString);

    ImageCandidateStoreEntryNotificationPlan plan;
    plan.failedLoads = takePendingLoads();
    plan.failedSubscribers = activeSubscribers();
    plan.errorString = m_errorString;
    return plan;
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

std::vector<ImageCandidateStoreEntryPendingLoad> ImageCandidateStoreEntryState::takePendingLoads()
{
    std::vector<ImageCandidateStoreEntryPendingLoad> pendingLoads = std::move(m_pendingLoads);
    m_pendingLoads.clear();
    return pendingLoads;
}

std::vector<ImageCandidateStoreEntrySubscriber> ImageCandidateStoreEntryState::activeSubscribers()
{
    pruneInactiveItems(&m_subscribers);
    return m_subscribers;
}
}
