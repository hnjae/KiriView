// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTANIMATIONLOADERRORPORT_H
#define KIRIVIEW_IMAGEDOCUMENTANIMATIONLOADERRORPORT_H

class QString;

namespace kiriview {
class ImageOpenController;

class ImageDocumentAnimationLoadErrorPort final
{
public:
    explicit ImageDocumentAnimationLoadErrorPort(ImageOpenController *openController = nullptr);

    void setOpenController(ImageOpenController *openController);
    void finishAnimationLoadWithError(const QString &errorString) const;

private:
    ImageOpenController *m_openController = nullptr;
};
}

#endif
