// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentdeletionstate.h"

namespace kiriview {
ImageDocumentDeletionState::ImageDocumentDeletionState(quint64 nextOperationId)
    : m_fileDeletion(nextOperationId)
{
}

bool ImageDocumentDeletionState::inProgress() const { return m_inProgress; }

ImageDocumentDeletionFileOperationStart ImageDocumentDeletionState::startFileDeletion()
{
    const quint64 operationId = m_fileDeletion.start();
    return ImageDocumentDeletionFileOperationStart {
        operationId,
        setInProgress(true),
    };
}

ImageDocumentDeletionFileOperationFinish ImageDocumentDeletionState::finishFileDeletion(
    quint64 operationId)
{
    if (!m_fileDeletion.finish(operationId)) {
        return {};
    }

    return ImageDocumentDeletionFileOperationFinish {
        true,
        setInProgress(false),
    };
}

bool ImageDocumentDeletionState::cancelFileDeletion()
{
    m_fileDeletion.cancel();
    return setInProgress(false);
}

bool ImageDocumentDeletionState::setInProgress(bool inProgress)
{
    if (m_inProgress == inProgress) {
        return false;
    }

    m_inProgress = inProgress;
    return true;
}
}
