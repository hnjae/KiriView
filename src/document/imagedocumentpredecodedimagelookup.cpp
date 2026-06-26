// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpredecodedimagelookup.h"

#include "imagedocumentpredecodecontroller.h"

#include <QUrl>
#include <utility>

namespace kiriview {
ImageDocumentPredecodedImageLookup::ImageDocumentPredecodedImageLookup(
    ExternalFinder externalFinder, const ImageDocumentPredecodeController* predecodeController)
    : m_externalFinder(std::move(externalFinder))
    , m_predecodeController(predecodeController)
{
}

std::optional<PredecodedImage> ImageDocumentPredecodedImageLookup::find(const QUrl& url) const
{
    if (m_externalFinder) {
        std::optional<PredecodedImage> predecoded = m_externalFinder(url);
        if (predecoded.has_value()) {
            return predecoded;
        }
    }

    if (m_predecodeController != nullptr) {
        return m_predecodeController->findPredecodedImage(url);
    }

    return std::nullopt;
}
}
