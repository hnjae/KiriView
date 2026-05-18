// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatedirectoryentry.h"

#include "imagenavigationmodel.h"

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

void notifyChanged(QObject *context,
    const std::shared_ptr<KiriView::ImageCandidateDirectoryEntry::Callbacks> &callbacks,
    const QString &key)
{
    QTimer::singleShot(0, context, [callbacks, key]() {
        if (callbacks->changed) {
            callbacks->changed(key);
        }
    });
}
}

namespace KiriView {
ImageCandidateDirectoryEntry::ImageCandidateDirectoryEntry(
    QUrl directoryUrl, QString key, QObject *signalContext, Callbacks callbacks)
    : m_directoryUrl(std::move(directoryUrl))
    , m_key(std::move(key))
    , m_lister(createLiveImageCandidateLister())
{
    connectSignals(signalContext, std::move(callbacks));
}

ImageCandidateDirectoryEntry::~ImageCandidateDirectoryEntry()
{
    QObject::disconnect(m_lister.get(), nullptr, nullptr, nullptr);
    m_lister->stop();
}

const QString &ImageCandidateDirectoryEntry::key() const { return m_key; }

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
    m_state.completeListing(imageCandidatesForLister(m_lister.get(), m_directoryUrl));
}

void ImageCandidateDirectoryEntry::handleChanged()
{
    m_state.updateListing(imageCandidatesForLister(m_lister.get(), m_directoryUrl));
}

void ImageCandidateDirectoryEntry::handleError(const QString &errorString)
{
    m_state.failListing(errorString);
}

ImageIoJob ImageCandidateDirectoryEntry::addPendingLoad(ImageCandidatesCallback callback,
    ErrorCallback errorCallback, QObject *receiver, std::function<void(QObject *)> removeToken)
{
    return m_state.addPendingLoad(receiver, m_lister.get(), std::move(callback),
        std::move(errorCallback), std::move(removeToken));
}

ImageIoJob ImageCandidateDirectoryEntry::addSubscriber(ImageCandidatesCallback callback,
    ErrorCallback errorCallback, QObject *receiver, std::function<void(QObject *)> removeToken)
{
    return m_state.addSubscriber(receiver, m_lister.get(), std::move(callback),
        std::move(errorCallback), std::move(removeToken));
}

void ImageCandidateDirectoryEntry::removePendingLoad(QObject *token)
{
    m_state.removePendingLoad(token);
}

void ImageCandidateDirectoryEntry::removeSubscriber(QObject *token)
{
    m_state.removeSubscriber(token);
}

void ImageCandidateDirectoryEntry::connectSignals(QObject *signalContext, Callbacks callbacks)
{
    QObject *context = signalContext == nullptr ? m_lister.get() : signalContext;
    auto sharedCallbacks = std::make_shared<Callbacks>(std::move(callbacks));

    QObject::connect(m_lister.get(), &KCoreDirLister::completed, context,
        [callbacks = sharedCallbacks, key = m_key]() {
            if (callbacks->completed) {
                callbacks->completed(key);
            }
        });
    QObject::connect(m_lister.get(), &KCoreDirLister::itemsAdded, context,
        [context, callbacks = sharedCallbacks, key = m_key](
            const QUrl &, const KFileItemList &) { notifyChanged(context, callbacks, key); });
    QObject::connect(m_lister.get(), &KCoreDirLister::itemsDeleted, context,
        [context, callbacks = sharedCallbacks, key = m_key](
            const KFileItemList &) { notifyChanged(context, callbacks, key); });
    QObject::connect(m_lister.get(), &KCoreDirLister::refreshItems, context,
        [context, callbacks = sharedCallbacks, key = m_key](
            const QList<QPair<KFileItem, KFileItem>> &) {
            notifyChanged(context, callbacks, key);
        });
    QObject::connect(m_lister.get(), &KCoreDirLister::clear, context,
        [context, callbacks = sharedCallbacks, key = m_key]() {
            notifyChanged(context, callbacks, key);
        });
    QObject::connect(m_lister.get(), &KCoreDirLister::clearDir, context,
        [context, callbacks = sharedCallbacks, key = m_key](
            const QUrl &) { notifyChanged(context, callbacks, key); });
    QObject::connect(m_lister.get(), &KCoreDirLister::jobError, context,
        [callbacks = sharedCallbacks, key = m_key](KIO::Job *job) {
            if (callbacks->error) {
                callbacks->error(key, job == nullptr ? QString() : job->errorString());
            }
        });
}
}
