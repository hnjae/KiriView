// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPREDECODEDIMAGELOOKUP_H
#define KIRIVIEW_IMAGEDOCUMENTPREDECODEDIMAGELOOKUP_H

#include "predecode/predecodedimage.h"

#include <functional>
#include <optional>

class QUrl;

namespace kiriview {
class ImageDocumentPredecodeController;

class ImageDocumentPredecodedImageLookup final
{
public:
    using ExternalFinder = std::function<std::optional<PredecodedImage>(const QUrl&)>;

    explicit ImageDocumentPredecodedImageLookup(ExternalFinder externalFinder = {},
        const ImageDocumentPredecodeController* predecodeController = nullptr);

    std::optional<PredecodedImage> find(const QUrl& url) const;

private:
    ExternalFinder m_externalFinder;
    const ImageDocumentPredecodeController* m_predecodeController = nullptr;
};
}

#endif
