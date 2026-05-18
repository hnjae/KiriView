// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletionstate.h"

namespace KiriView {
ImageDocumentDeletionState::ImageDocumentDeletionState(quint64 nextOperationId)
    : m_nextOperationId(nextOperationId)
{
}

bool ImageDocumentDeletionState::inProgress() const { return m_inProgress; }

ImageDocumentDeletionFileOperationStart ImageDocumentDeletionState::startFileDeletion()
{
    const quint64 operationId = nextOperationId();
    m_fileDeletionOperationId = operationId;
    return ImageDocumentDeletionFileOperationStart {
        operationId,
        setInProgress(true),
    };
}

ImageDocumentDeletionFileOperationFinish ImageDocumentDeletionState::finishFileDeletion(
    quint64 operationId)
{
    if (!acceptsFileDeletion(operationId)) {
        return {};
    }

    m_fileDeletionOperationId = 0;
    return ImageDocumentDeletionFileOperationFinish {
        true,
        setInProgress(false),
    };
}

bool ImageDocumentDeletionState::cancelFileDeletion()
{
    m_fileDeletionOperationId = 0;
    return setInProgress(false);
}

quint64 ImageDocumentDeletionState::startFallback()
{
    m_fallbackOperationId = nextOperationId();
    return m_fallbackOperationId;
}

bool ImageDocumentDeletionState::acceptsFallback(quint64 operationId) const
{
    return operationId != 0 && operationId == m_fallbackOperationId;
}

bool ImageDocumentDeletionState::finishFallback(quint64 operationId)
{
    if (!acceptsFallback(operationId)) {
        return false;
    }

    m_fallbackOperationId = 0;
    return true;
}

void ImageDocumentDeletionState::cancelFallback() { m_fallbackOperationId = 0; }

quint64 ImageDocumentDeletionState::nextOperationId()
{
    ++m_nextOperationId;
    if (m_nextOperationId == 0) {
        ++m_nextOperationId;
    }
    return m_nextOperationId;
}

bool ImageDocumentDeletionState::setInProgress(bool inProgress)
{
    if (m_inProgress == inProgress) {
        return false;
    }

    m_inProgress = inProgress;
    return true;
}

bool ImageDocumentDeletionState::acceptsFileDeletion(quint64 operationId) const
{
    return operationId != 0 && operationId == m_fileDeletionOperationId;
}
}
