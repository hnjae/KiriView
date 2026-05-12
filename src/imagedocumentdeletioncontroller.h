// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTDELETIONCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTDELETIONCONTROLLER_H

#include "filedeletion.h"

namespace KiriView {
class ImageDeletionController;
class ImageDocumentState;
class ImagePresentationController;

class ImageDocumentDeletionController final
{
public:
    ImageDocumentDeletionController(ImageDocumentState &state,
        ImagePresentationController &presentationController,
        ImageDeletionController &deletionController);

    bool inProgress() const;
    void deleteDisplayedFile(FileDeletionMode mode);
    void cancel();

private:
    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    ImageDeletionController &m_deletionController;
};
}

#endif
