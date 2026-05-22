// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatedirectoryentry.h"

#include "async/imagecallback.h"
#include "imagecandidateitems.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <QTimer>
#include <memory>
#include <utility>

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

std::vector<KiriView::ImageNavigationCandidate> imageCandidatesForLister(
    KCoreDirLister *lister, const QUrl &directoryUrl)
{
    return KiriView::imageNavigationCandidates(
        lister->itemsForDir(directoryUrl, KCoreDirLister::AllItems));
}

QObject *createEntryJobToken(QObject *receiver, QObject *fallbackParent)
{
    return new QObject(receiver == nullptr ? fallbackParent : receiver);
}

KiriView::ImageIoJob createEntryJob(QObject *token, std::function<void(QObject *)> removeToken)
{
    return KiriView::ImageIoJob(token, [removeToken = std::move(removeToken)](QObject *object) {
        removeToken(object);
        object->deleteLater();
    });
}

void notifyChanged(QObject *context, KiriView::ImageCandidateDirectoryEntry *entry)
{
    QTimer::singleShot(0, context, [entry]() {
        if (entry != nullptr) {
            entry->handleChanged();
        }
    });
}

void applyEntryNotificationPlan(KiriView::ImageCandidateStoreEntryNotificationPlan plan)
{
    for (const KiriView::ImageCandidateStoreEntryPendingLoad &load : plan.completedLoads) {
        load.completion.claimAndDelete(
            [&]() { KiriView::invokeIfSet(load.callback, plan.candidates); });
    }
    for (const KiriView::ImageCandidateStoreEntrySubscriber &subscriber : plan.changedSubscribers) {
        KiriView::invokeIfSet(subscriber.callback, plan.candidates);
    }
    for (const KiriView::ImageCandidateStoreEntryPendingLoad &load : plan.failedLoads) {
        load.completion.claimAndDelete(
            [&]() { KiriView::invokeIfSet(load.errorCallback, plan.errorString); });
    }
    for (const KiriView::ImageCandidateStoreEntrySubscriber &subscriber : plan.failedSubscribers) {
        KiriView::invokeIfSet(subscriber.errorCallback, plan.errorString);
    }
}
}

namespace KiriView {
ImageCandidateDirectoryEntry::ImageCandidateDirectoryEntry(
    QUrl directoryUrl, QObject *signalContext)
    : m_directoryUrl(std::move(directoryUrl))
    , m_lister(createLiveImageCandidateLister())
{
    connectSignals(signalContext);
}

ImageCandidateDirectoryEntry::~ImageCandidateDirectoryEntry()
{
    QObject::disconnect(m_lister.get(), nullptr, nullptr, nullptr);
    m_lister->stop();
}

bool ImageCandidateDirectoryEntry::failed() const { return m_state.failed(); }

bool ImageCandidateDirectoryEntry::listed() const { return m_state.listed(); }

const QString &ImageCandidateDirectoryEntry::errorString() const { return m_state.errorString(); }

const std::vector<ImageNavigationCandidate> &ImageCandidateDirectoryEntry::candidates() const
{
    return m_state.candidates();
}

bool ImageCandidateDirectoryEntry::open()
{
    return m_lister->openUrl(m_directoryUrl, KCoreDirLister::Reload);
}

void ImageCandidateDirectoryEntry::handleCompleted()
{
    applyEntryNotificationPlan(
        m_state.completeListing(imageCandidatesForLister(m_lister.get(), m_directoryUrl)));
}

void ImageCandidateDirectoryEntry::handleChanged()
{
    applyEntryNotificationPlan(
        m_state.updateListing(imageCandidatesForLister(m_lister.get(), m_directoryUrl)));
}

void ImageCandidateDirectoryEntry::handleError(const QString &errorString)
{
    applyEntryNotificationPlan(m_state.failListing(errorString));
}

ImageIoJob ImageCandidateDirectoryEntry::addPendingLoad(ImageCandidatesCallback callback,
    ErrorCallback errorCallback, QObject *receiver, std::function<void(QObject *)> removeToken)
{
    QObject *token = createEntryJobToken(receiver, m_lister.get());
    ImageIoJob job = createEntryJob(token, std::move(removeToken));
    m_state.addPendingLoad(job.completion(), std::move(callback), std::move(errorCallback));
    return job;
}

ImageIoJob ImageCandidateDirectoryEntry::addSubscriber(ImageCandidatesCallback callback,
    ErrorCallback errorCallback, QObject *receiver, std::function<void(QObject *)> removeToken)
{
    QObject *token = createEntryJobToken(receiver, m_lister.get());
    ImageIoJob job = createEntryJob(token, std::move(removeToken));
    m_state.addSubscriber(token, std::move(callback), std::move(errorCallback));
    return job;
}

void ImageCandidateDirectoryEntry::removePendingLoad(QObject *token)
{
    m_state.removePendingLoad(token);
}

void ImageCandidateDirectoryEntry::removeSubscriber(QObject *token)
{
    m_state.removeSubscriber(token);
}

void ImageCandidateDirectoryEntry::connectSignals(QObject *signalContext)
{
    QObject *context = signalContext == nullptr ? m_lister.get() : signalContext;

    QObject::connect(
        m_lister.get(), &KCoreDirLister::completed, context, [this]() { handleCompleted(); });
    QObject::connect(m_lister.get(), &KCoreDirLister::itemsAdded, context,
        [this, context](const QUrl &, const KFileItemList &) { notifyChanged(context, this); });
    QObject::connect(m_lister.get(), &KCoreDirLister::itemsDeleted, context,
        [this, context](const KFileItemList &) { notifyChanged(context, this); });
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
