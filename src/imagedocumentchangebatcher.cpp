// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentchangebatcher.h"

#include "imagecallback.h"

#include <algorithm>
#include <utility>

namespace KiriView {
ImageDocumentChangeBatcher::Batch::Batch(ImageDocumentChangeBatcher &batcher)
    : m_batcher(&batcher)
{
    m_batcher->begin();
}

ImageDocumentChangeBatcher::Batch::~Batch()
{
    if (m_batcher != nullptr) {
        m_batcher->end();
    }
}

ImageDocumentChangeBatcher::Batch::Batch(Batch &&other) noexcept
    : m_batcher(other.m_batcher)
{
    other.m_batcher = nullptr;
}

ImageDocumentChangeBatcher::ImageDocumentChangeBatcher(ChangeCallback changeCallback)
    : m_changeCallback(std::move(changeCallback))
{
}

ImageDocumentChangeBatcher::Batch ImageDocumentChangeBatcher::beginBatch() { return Batch(*this); }

void ImageDocumentChangeBatcher::notify(ImageDocumentChange change)
{
    if (m_batchDepth > 0) {
        const bool alreadyPending
            = std::find(m_pendingChanges.cbegin(), m_pendingChanges.cend(), change)
            != m_pendingChanges.cend();
        if (!alreadyPending) {
            m_pendingChanges.push_back(change);
        }
        return;
    }

    emitChange(change);
}

void ImageDocumentChangeBatcher::notifyAll(const std::vector<ImageDocumentChange> &changes)
{
    for (ImageDocumentChange change : changes) {
        notify(change);
    }
}

void ImageDocumentChangeBatcher::begin() { ++m_batchDepth; }

void ImageDocumentChangeBatcher::end()
{
    if (m_batchDepth <= 0) {
        return;
    }

    --m_batchDepth;
    if (m_batchDepth > 0) {
        return;
    }

    std::vector<ImageDocumentChange> changes = std::move(m_pendingChanges);
    m_pendingChanges.clear();
    for (ImageDocumentChange change : changes) {
        emitChange(change);
    }
}

void ImageDocumentChangeBatcher::emitChange(ImageDocumentChange change)
{
    invokeIfSet(m_changeCallback, change);
}
}
