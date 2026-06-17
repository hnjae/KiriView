// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTDELETIONPROGRESSPORT_H
#define KIRIVIEW_IMAGEDOCUMENTDELETIONPROGRESSPORT_H

namespace kiriview {
class ImageDocumentDeletionController;

class ImageDocumentDeletionProgressPort final
{
public:
    explicit ImageDocumentDeletionProgressPort(
        const ImageDocumentDeletionController *deletionController = nullptr);

    bool inProgress() const;

private:
    const ImageDocumentDeletionController *m_deletionController = nullptr;
};
}

#endif
