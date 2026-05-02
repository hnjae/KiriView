// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNAVIGATIONCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTNAVIGATIONCONTROLLER_H

#include "imagedocumentevents.h"
#include "imagedocumenttypes.h"
#include "imagenavigationservice.h"

#include <QString>
#include <QUrl>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
class ImageDocumentState;
class ImagePresentationController;

class ImageDocumentNavigationController final
{
public:
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using EventCallback = std::function<void(DocumentEvent)>;

    ImageDocumentNavigationController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController, ChangeCallback changeCallback,
        EventCallback eventCallback);
    ~ImageDocumentNavigationController();

    int currentPageNumber() const;
    int imageCount() const;

    void openPreviousImage();
    void openNextImage();
    void openImageAtPage(int pageNumber);
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
    void notify(ImageDocumentChange change);

    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    ChangeCallback m_changeCallback;
    EventCallback m_eventCallback;
    std::unique_ptr<ImageNavigationService> m_navigationService;
};
}

#endif
