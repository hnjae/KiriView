// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTDELETIONSTATE_H
#define KIRIVIEW_IMAGEDOCUMENTDELETIONSTATE_H

#include <QtGlobal>

namespace KiriView {
struct ImageDocumentDeletionFileOperationStart {
    quint64 operationId = 0;
    bool inProgressChanged = false;
};

struct ImageDocumentDeletionFileOperationFinish {
    bool accepted = false;
    bool inProgressChanged = false;
};

class ImageDocumentDeletionState final
{
public:
    explicit ImageDocumentDeletionState(quint64 nextOperationId = 0);

    bool inProgress() const;
    ImageDocumentDeletionFileOperationStart startFileDeletion();
    ImageDocumentDeletionFileOperationFinish finishFileDeletion(quint64 operationId);
    bool cancelFileDeletion();

    quint64 startFallback();
    bool acceptsFallback(quint64 operationId) const;
    bool finishFallback(quint64 operationId);
    void cancelFallback();

private:
    quint64 nextOperationId();
    bool setInProgress(bool inProgress);
    bool acceptsFileDeletion(quint64 operationId) const;

    bool m_inProgress = false;
    quint64 m_nextOperationId = 0;
    quint64 m_fileDeletionOperationId = 0;
    quint64 m_fallbackOperationId = 0;
};
}

#endif
