// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H

#include "imagedocumentsourceloadexecutor.h"
#include "imagedocumentsourceloadrequest.h"

namespace KiriView {
class ArchiveDocumentSessionStore;
class ImageDocumentDeletionController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
class ImageOpenController;
class ImageSpreadPresentationController;
class ImageNavigationService;

class ImageDocumentLoadController final
{
public:
    ImageDocumentLoadController(ImageDocumentState &state,
        ImageDocumentDeletionController &deletionController,
        ImageNavigationService &navigationService,
        ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
        ImageSpreadPresentationController &spreadController,
        ArchiveDocumentSessionStore *archiveSessionStore = nullptr);

    void loadSource(const ImageDocumentSourceLoadRequest &request);

private:
    ImageDocumentSourceLoadOperations sourceLoadOperations();

    ImageDocumentState &m_state;
    ImageDocumentDeletionController &m_deletionController;
    ImageNavigationService &m_navigationService;
    ImageDocumentPredecodeController &m_predecodeController;
    ImageOpenController &m_openController;
    ImageSpreadPresentationController &m_spreadController;
    ArchiveDocumentSessionStore *m_archiveSessionStore = nullptr;
};
}

#endif
