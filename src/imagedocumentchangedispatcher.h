// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTCHANGEDISPATCHER_H
#define KIRIVIEW_IMAGEDOCUMENTCHANGEDISPATCHER_H

#include "imagedocumenttypes.h"

#include <functional>

namespace KiriView {
class ImageDocumentState;
class ImageSpreadPresentationController;

class ImageDocumentChangeDispatcher final
{
public:
    using ChangeCallback = std::function<void(ImageDocumentChange)>;

    ImageDocumentChangeDispatcher(ImageDocumentState &state,
        ImageSpreadPresentationController &spreadController, ChangeCallback changeCallback);

    void dispatch(ImageDocumentChange change);

private:
    ImageDocumentState &m_state;
    ImageSpreadPresentationController &m_spreadController;
    ChangeCallback m_changeCallback;
};
}

#endif
