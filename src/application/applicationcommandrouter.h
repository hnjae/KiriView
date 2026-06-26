// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONCOMMANDROUTER_H
#define KIRIVIEW_APPLICATIONCOMMANDROUTER_H

#include "application/applicationtypes.h"
#include "navigation/imageshortcutnavigationpolicy.h"

#include <QtGlobal>
#include <functional>

namespace kiriview::ApplicationActions {
struct ApplicationCommandRouterInput
{
    bool imagePannable = false;
    bool rightToLeftReadingActive = false;
    bool videoMode = false;
    bool imageDocumentPageNavigationActive = false;
    bool atKnownFirstActiveNavigation = false;
    bool canOpenPreviousActiveNavigation = false;
};

struct ApplicationCommandRouterShellPorts
{
    std::function<void()> requestOpenDialog;
    std::function<void()> openApplicationMenu;
};

struct ApplicationCommandRouterSessionPorts
{
    std::function<void()> openCurrentMediaWith;
    std::function<void()> moveDisplayedFileToTrash;
    std::function<void()> deleteDisplayedFilePermanently;
    std::function<void()> requestPreviousActiveNavigationWithBoundary;
    std::function<void()> requestNextActiveNavigationWithBoundary;
    std::function<void()> openFirstActiveNavigation;
    std::function<void()> openLastActiveNavigation;
    std::function<void()> showFirstImageBoundary;
};

struct ApplicationCommandRouterImageDocumentPorts
{
    std::function<bool()> imageAvailable;
    std::function<void()> openPreviousContainer;
    std::function<void()> openNextContainer;
    std::function<void()> openPreviousSinglePage;
    std::function<void()> openNextSinglePage;
    std::function<void()> rotateClockwise;
    std::function<void()> rotateCounterclockwise;
    std::function<void()> requestToggleTwoPageMode;
    std::function<void()> requestToggleRightToLeftReading;
};

struct ApplicationCommandRouterImagePresentationPorts
{
    std::function<bool()> imageViewportHorizontallyPannable;
    std::function<void(double, double)> requestViewportPanBy;
    std::function<bool()> requestViewportScanForward;
    std::function<bool()> requestViewportScanBackward;
    std::function<void()> requestViewportPanToInitialScanPosition;
    std::function<void()> requestViewportPanToFinalScanPosition;
    std::function<void()> requestNextDisplayedImageStartToFinalScanPosition;
    std::function<void(double)> requestZoomByStepAtCenter;
    std::function<void(double)> requestManualZoomPercent;
    std::function<void()> requestFitMode;
    std::function<void()> requestFitHeightMode;
    std::function<void()> requestFitWidthMode;
};

struct ApplicationCommandRouterPanelPorts
{
    std::function<void()> toggleInfoPanel;
    std::function<void()> toggleThumbnailPanel;
};

struct ApplicationCommandRouterWindowPorts
{
    std::function<void()> toggleFullScreen;
};

struct ApplicationCommandRouterHelpPorts
{
    std::function<void()> requestShortcutHelp;
};

struct ApplicationCommandRouterVideoPorts
{
    std::function<bool()> videoAvailable;
    std::function<bool()> videoSeekable;
    std::function<qint64()> videoDuration;
    std::function<void(qint64)> seekVideoBy;
    std::function<void(qint64)> setVideoPosition;
    std::function<void()> toggleVideoPlayback;
};

struct ApplicationCommandRouterPorts
{
    ApplicationCommandRouterShellPorts shell;
    ApplicationCommandRouterSessionPorts session;
    ApplicationCommandRouterImageDocumentPorts imageDocument;
    ApplicationCommandRouterImagePresentationPorts imagePresentation;
    ApplicationCommandRouterPanelPorts panel;
    ApplicationCommandRouterWindowPorts window;
    ApplicationCommandRouterHelpPorts help;
    ApplicationCommandRouterVideoPorts video;
};

class ApplicationCommandRouter final
{
public:
    void handleActionTriggered(ActionId actionId, ApplicationCommandRouterInput input,
        const ApplicationCommandRouterPorts& ports) const;
    void handleScanForwardAction(
        ApplicationCommandRouterInput input, const ApplicationCommandRouterPorts& ports) const;
    void handleScanBackwardAction(
        ApplicationCommandRouterInput input, const ApplicationCommandRouterPorts& ports) const;
    bool executeHorizontalArrowShortcut(ApplicationCommandRouterInput input,
        const ApplicationCommandRouterPorts& ports, bool leftArrow) const;
    bool executeSinglePageArrowShortcut(ApplicationCommandRouterInput input,
        const ApplicationCommandRouterPorts& ports, bool leftArrow) const;
    bool executeVerticalPanShortcut(ApplicationCommandRouterInput input,
        const ApplicationCommandRouterPorts& ports, bool up) const;
    bool executeVideoSeekShortcut(ApplicationCommandRouterInput input,
        const ApplicationCommandRouterPorts& ports, qint64 deltaMilliseconds) const;

private:
    kiriview::ImageShortcutNavigationPolicy m_navigationPolicy;
};
}

#endif
