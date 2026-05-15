// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPREDECODECONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTPREDECODECONTROLLER_H

#include "imageasyncdependencies.h"
#include "predecodedimage.h"

#include <QUrl>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class ImageDocumentState;
class ImagePredecodeCoordinator;
class ImagePresentationController;

class ImageDocumentPredecodeController final
{
public:
    ImageDocumentPredecodeController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController,
        ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies);
    ~ImageDocumentPredecodeController();

    void scheduleAdjacentImagePredecode(
        std::optional<DisplayedPredecodeImage> secondaryImage = std::nullopt);
    void cancel();
    void clear();
    std::optional<PredecodedImage> tryTake(const QUrl &url) const;

private:
    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    std::unique_ptr<ImagePredecodeCoordinator> m_coordinator;
};
}

#endif
