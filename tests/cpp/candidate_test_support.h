// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_TESTS_CANDIDATE_TEST_SUPPORT_H
#define KIRIVIEW_TESTS_CANDIDATE_TEST_SUPPORT_H

#include "async/imageiojob.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <algorithm>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace kiriview::TestSupport {
inline QString keyForUrl(const QUrl &url)
{
    return url.adjusted(QUrl::NormalizePathSegments).toString();
}

inline QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

inline QString indexedImageFileName(int index)
{
    return QStringLiteral("%1.png").arg(index, 2, 10, QLatin1Char('0'));
}

inline QUrl indexedImageUrl(int index)
{
    return localUrl(QStringLiteral("/images/") + indexedImageFileName(index));
}

inline QUrl imagesDirectoryUrl() { return localUrl(QStringLiteral("/images/")); }

inline QUrl archivePageUrl(const QUrl &archiveRootUrl, const QString &pageName)
{
    QUrl pageUrl = archiveRootUrl;
    pageUrl.setPath(archiveRootUrl.path() + pageName);
    return pageUrl;
}

inline ImageDocumentPageCandidate imageDocumentPageCandidate(const QUrl &url)
{
    return ImageDocumentPageCandidate { url, url.fileName() };
}

inline ImageDocumentPageCandidate videoCandidate(const QUrl &url)
{
    return ImageDocumentPageCandidate {
        url,
        url.fileName(),
        ImageDocumentPageKind::Video,
    };
}

inline ContainerNavigationCandidate containerCandidate(
    const QUrl &url, ContainerNavigationCandidateType type)
{
    return ContainerNavigationCandidate { url, url.fileName(), type };
}

inline ContainerNavigationCandidate comicBookContainerCandidate(const QUrl &url)
{
    return containerCandidate(url, ContainerNavigationCandidateType::ComicBookArchive);
}

template <typename Candidates> class FakeCandidateListing
{
public:
    void setItems(const QUrl &url, Candidates candidates)
    {
        m_itemsByUrl[keyForUrl(url)] = std::move(candidates);
    }

    void setError(const QUrl &url, QString errorString)
    {
        m_errorsByUrl[keyForUrl(url)] = std::move(errorString);
    }

    template <typename Callback>
    void load(QUrl url, Callback callback, ErrorCallback errorCallback) const
    {
        const QString key = keyForUrl(url);
        const auto error = m_errorsByUrl.find(key);
        if (error != m_errorsByUrl.cend()) {
            if (errorCallback) {
                errorCallback(error->second);
            }
            return;
        }

        const auto items = m_itemsByUrl.find(key);
        if (items == m_itemsByUrl.cend()) {
            if (errorCallback) {
                errorCallback(
                    QStringLiteral("missing fake candidate listing for %1").arg(url.toString()));
            }
            return;
        }

        if (callback) {
            callback(items->second);
        }
    }

private:
    std::map<QString, Candidates> m_itemsByUrl;
    std::map<QString, QString> m_errorsByUrl;
};

class FakeImageDocumentPageCandidateProvider
{
    struct FakeCandidateChangeSubscription;

public:
    void setDirectoryImages(
        const QUrl &directoryUrl, std::vector<ImageDocumentPageCandidate> candidates)
    {
        m_directoryImageDocumentPages.setItems(directoryUrl, std::move(candidates));
    }

    void setOpenedCollectionCandidates(
        const QUrl &archiveRootUrl, std::vector<ImageDocumentPageCandidate> candidates)
    {
        m_openedCollectionCandidates.setItems(archiveRootUrl, std::move(candidates));
    }

    void setContainerCandidates(
        const QUrl &directoryUrl, std::vector<ContainerNavigationCandidate> candidates)
    {
        m_containerCandidates.setItems(directoryUrl, std::move(candidates));
    }

    void setDirectoryImageError(const QUrl &directoryUrl, QString errorString)
    {
        m_directoryImageDocumentPages.setError(directoryUrl, std::move(errorString));
    }

    void setOpenedCollectionCandidateError(const QUrl &archiveRootUrl, QString errorString)
    {
        m_openedCollectionCandidates.setError(archiveRootUrl, std::move(errorString));
    }

    void setContainerError(const QUrl &directoryUrl, QString errorString)
    {
        m_containerCandidates.setError(directoryUrl, std::move(errorString));
    }

    void emitDirectoryImageChanges(
        const QUrl &directoryUrl, std::vector<ImageDocumentPageCandidate> candidates)
    {
        const QString key = keyForUrl(directoryUrl);
        for (const std::shared_ptr<FakeCandidateChangeSubscription> &subscription :
            m_directoryImageChangeSubscriptions) {
            if (subscription == nullptr || subscription->canceled || subscription->key != key
                || !subscription->callback) {
                continue;
            }

            subscription->callback(candidates);
        }
    }

    int directoryImageChangeSubscriptionCount(const QUrl &directoryUrl) const
    {
        const QString key = keyForUrl(directoryUrl);
        return static_cast<int>(std::count_if(m_directoryImageChangeSubscriptions.cbegin(),
            m_directoryImageChangeSubscriptions.cend(),
            [&key](const std::shared_ptr<FakeCandidateChangeSubscription> &subscription) {
                return subscription != nullptr && !subscription->canceled
                    && subscription->key == key;
            }));
    }

    int directoryImageChangeSubscriptionCount() const
    {
        return static_cast<int>(std::count_if(m_directoryImageChangeSubscriptions.cbegin(),
            m_directoryImageChangeSubscriptions.cend(),
            [](const std::shared_ptr<FakeCandidateChangeSubscription> &subscription) {
                return subscription != nullptr && !subscription->canceled;
            }));
    }

    ImageDocumentPageCandidateProvider provider()
    {
        return ImageDocumentPageCandidateProvider {
            [this](QObject *, QUrl directoryUrl, ImageDocumentPageCandidatesCallback callback,
                ErrorCallback errorCallback) {
                m_directoryImageDocumentPages.load(
                    std::move(directoryUrl), std::move(callback), std::move(errorCallback));
                return ImageIoJob();
            },
            [this](QObject *, QUrl directoryUrl, ContainerCandidatesCallback callback,
                ErrorCallback errorCallback) {
                m_containerCandidates.load(
                    std::move(directoryUrl), std::move(callback), std::move(errorCallback));
                return ImageIoJob();
            },
            [this](QObject *, OpenedCollectionScopeLocation openedCollectionScope,
                ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback) {
                m_openedCollectionCandidates.load(
                    openedCollectionScope.rootUrl(), std::move(callback), std::move(errorCallback));
                return ImageIoJob();
            },
            [this](QObject *receiver, QUrl directoryUrl,
                ImageDocumentPageCandidatesCallback callback, ErrorCallback) {
                return subscribeToDirectoryImageChanges(
                    receiver, std::move(directoryUrl), std::move(callback));
            },
        };
    }

private:
    struct FakeCandidateChangeSubscription {
        QObject *object = nullptr;
        QString key;
        ImageDocumentPageCandidatesCallback callback;
        bool canceled = false;
    };

    ImageIoJob subscribeToDirectoryImageChanges(
        QObject *receiver, QUrl directoryUrl, ImageDocumentPageCandidatesCallback callback)
    {
        auto subscription = std::make_shared<FakeCandidateChangeSubscription>();
        subscription->object = new QObject(receiver);
        subscription->key = keyForUrl(directoryUrl);
        subscription->callback = std::move(callback);

        std::weak_ptr<FakeCandidateChangeSubscription> weakSubscription = subscription;
        ImageIoJob job(subscription->object, [weakSubscription](QObject *object) {
            if (std::shared_ptr<FakeCandidateChangeSubscription> subscription
                = weakSubscription.lock()) {
                subscription->canceled = true;
                subscription->object = nullptr;
            }
            if (object != nullptr) {
                object->deleteLater();
            }
        });
        m_directoryImageChangeSubscriptions.push_back(std::move(subscription));
        return job;
    }

    FakeCandidateListing<std::vector<ImageDocumentPageCandidate>> m_directoryImageDocumentPages;
    FakeCandidateListing<std::vector<ImageDocumentPageCandidate>> m_openedCollectionCandidates;
    FakeCandidateListing<std::vector<ContainerNavigationCandidate>> m_containerCandidates;
    std::vector<std::shared_ptr<FakeCandidateChangeSubscription>>
        m_directoryImageChangeSubscriptions;
};
}

#endif
