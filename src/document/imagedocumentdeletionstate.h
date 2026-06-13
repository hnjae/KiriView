// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTDELETIONSTATE_H
#define KIRIVIEW_IMAGEDOCUMENTDELETIONSTATE_H

#include "async/imageasyncoperationstate.h"

namespace kiriview {
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

private:
    bool setInProgress(bool inProgress);

    bool m_inProgress = false;
    ImageAsyncOperationState m_fileDeletion;
};
}

#endif
