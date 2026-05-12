// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNAVIGATIONCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTNAVIGATIONCONTROLLER_H

#include "imagedocumenteffects.h"
#include "imagedocumenttypes.h"
#include "imagenavigationservice.h"

#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class ImageDocumentState;
class ImagePresentationController;

class ImageDocumentNavigationController final
{
public:
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using EffectCallback = std::function<void(ImageDocumentEffect)>;

    struct Callbacks {
        ChangeCallback change;
        EffectCallback effect;
    };

    ImageDocumentNavigationController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController, Callbacks callbacks,
        ImageNavigationCandidateProvider candidateProvider);
    ~ImageDocumentNavigationController();

    int currentPageNumber() const;
    int imageCount() const;
    std::optional<QUrl> urlAtPage(int pageNumber) const;

    void openPreviousImage();
    void openNextImage();
    void openPreviousContainer();
    void openNextContainer();

    void updatePageNavigation();
    void cancelNavigation();
    void cancelContainerNavigation();
    void cancelPageNavigationUpdate();
    void clearPageNavigation();

private:
    ImageNavigationService::DisplayContext displayContext() const;
    void openAdjacentImage(NavigationDirection direction);
    void openAdjacentContainer(NavigationDirection direction);
    void report(ImageDocumentEffect effect);
    void notify(ImageDocumentChange change);

    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    Callbacks m_callbacks;
    std::unique_ptr<ImageNavigationService> m_navigationService;
};
}

#endif
