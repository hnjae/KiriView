// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagecandidatestoreentrystate.h"

#include "location/imageurl.h"

#include <algorithm>
#include <utility>

namespace {
bool sameImageDocumentPageCandidates(const std::vector<KiriView::ImageDocumentPageCandidate> &left,
    const std::vector<KiriView::ImageDocumentPageCandidate> &right)
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
const std::vector<ImageDocumentPageCandidate> &
ImageDocumentPageCandidateStoreEntryState::candidates() const
{
    return m_candidates;
}

bool ImageDocumentPageCandidateStoreEntryState::listed() const { return m_listed; }

bool ImageDocumentPageCandidateStoreEntryState::failed() const { return m_failed; }

const QString &ImageDocumentPageCandidateStoreEntryState::errorString() const
{
    return m_errorString;
}

void ImageDocumentPageCandidateStoreEntryState::addPendingLoad(ImageIoJobCompletion completion,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback)
{
    m_pendingLoads.push_back(ImageDocumentPageCandidateStoreEntryPendingLoad {
        std::move(completion),
        std::move(callback),
        std::move(errorCallback),
    });
}

void ImageDocumentPageCandidateStoreEntryState::addSubscriber(
    QObject *token, ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback)
{
    m_subscribers.push_back(ImageDocumentPageCandidateStoreEntrySubscriber {
        token,
        std::move(callback),
        std::move(errorCallback),
    });
}

void ImageDocumentPageCandidateStoreEntryState::removePendingLoad(QObject *token)
{
    const auto removed = std::remove_if(m_pendingLoads.begin(), m_pendingLoads.end(),
        [token](const ImageDocumentPageCandidateStoreEntryPendingLoad &load) {
            return load.completion.object() == token;
        });
    m_pendingLoads.erase(removed, m_pendingLoads.end());
}

void ImageDocumentPageCandidateStoreEntryState::removeSubscriber(QObject *token)
{
    const auto removed = std::remove_if(m_subscribers.begin(), m_subscribers.end(),
        [token](const ImageDocumentPageCandidateStoreEntrySubscriber &subscriber) {
            return subscriber.token.data() == token;
        });
    m_subscribers.erase(removed, m_subscribers.end());
}

ImageDocumentPageCandidateStoreEntryNotificationPlan
ImageDocumentPageCandidateStoreEntryState::completeListing(
    std::vector<ImageDocumentPageCandidate> candidates)
{
    const bool wasListed = m_listed;
    const bool changed = replaceCandidates(std::move(candidates));
    m_listed = true;
    m_failed = false;
    m_errorString = QString();

    ImageDocumentPageCandidateStoreEntryNotificationPlan plan;
    plan.completedLoads = takePendingLoads();
    plan.candidates = m_candidates;
    if (wasListed && changed) {
        plan.changedSubscribers = activeSubscribers();
    }
    return plan;
}

ImageDocumentPageCandidateStoreEntryNotificationPlan
ImageDocumentPageCandidateStoreEntryState::updateListing(
    std::vector<ImageDocumentPageCandidate> candidates)
{
    const bool changed = replaceCandidates(std::move(candidates));
    ImageDocumentPageCandidateStoreEntryNotificationPlan plan;
    plan.candidates = m_candidates;
    if (m_listed && changed) {
        plan.changedSubscribers = activeSubscribers();
    }
    return plan;
}

ImageDocumentPageCandidateStoreEntryNotificationPlan
ImageDocumentPageCandidateStoreEntryState::failListing(QString errorString)
{
    m_failed = true;
    m_errorString = std::move(errorString);

    ImageDocumentPageCandidateStoreEntryNotificationPlan plan;
    plan.failedLoads = takePendingLoads();
    plan.failedSubscribers = activeSubscribers();
    plan.errorString = m_errorString;
    return plan;
}

bool ImageDocumentPageCandidateStoreEntryState::replaceCandidates(
    std::vector<ImageDocumentPageCandidate> candidates)
{
    if (sameImageDocumentPageCandidates(m_candidates, candidates)) {
        return false;
    }

    m_candidates = std::move(candidates);
    return true;
}

std::vector<ImageDocumentPageCandidateStoreEntryPendingLoad>
ImageDocumentPageCandidateStoreEntryState::takePendingLoads()
{
    std::vector<ImageDocumentPageCandidateStoreEntryPendingLoad> pendingLoads
        = std::move(m_pendingLoads);
    m_pendingLoads.clear();
    return pendingLoads;
}

std::vector<ImageDocumentPageCandidateStoreEntrySubscriber>
ImageDocumentPageCandidateStoreEntryState::activeSubscribers()
{
    pruneInactiveItems(&m_subscribers);
    return m_subscribers;
}
}
