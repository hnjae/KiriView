// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONCOMMANDROUTER_H
#define KIRIVIEW_APPLICATIONCOMMANDROUTER_H

#include "application/applicationtypes.h"
#include "navigation/imageshortcutnavigationpolicy.h"

#include <QtGlobal>
#include <functional>

namespace KiriView::ApplicationActions {
struct ApplicationCommandRouterInput {
    bool imagePannable = false;
    bool rightToLeftReadingActive = false;
    bool videoMode = false;
    bool imageDocumentPageNavigationActive = false;
    bool atKnownFirstActiveNavigation = false;
    bool canOpenPreviousActiveNavigation = false;
};

struct ApplicationCommandRouterPorts {
    std::function<void()> requestOpenDialog;
    std::function<void()> openCurrentMediaWith;
    std::function<void()> moveDisplayedFileToTrash;
    std::function<void()> deleteDisplayedFilePermanently;
    std::function<bool()> imageAvailable;
    std::function<void()> openPreviousContainer;
    std::function<void()> openNextContainer;
    std::function<bool()> imageViewportHorizontallyPannable;
    std::function<void(double, double)> requestViewportPanBy;
    std::function<bool()> requestViewportScanForward;
    std::function<bool()> requestViewportScanBackward;
    std::function<void()> requestViewportPanToInitialScanPosition;
    std::function<void()> requestViewportPanToFinalScanPosition;
    std::function<void()> requestNextDisplayedImageStartToFinalScanPosition;
    std::function<void()> openPreviousSinglePage;
    std::function<void()> openNextSinglePage;
    std::function<void()> requestPreviousActiveNavigationWithBoundary;
    std::function<void()> requestNextActiveNavigationWithBoundary;
    std::function<void()> openFirstActiveNavigation;
    std::function<void()> openLastActiveNavigation;
    std::function<void(double)> requestZoomByStepAtCenter;
    std::function<void(double)> requestManualZoomPercent;
    std::function<void()> requestFitMode;
    std::function<void()> requestFitHeightMode;
    std::function<void()> requestFitWidthMode;
    std::function<void()> rotateClockwise;
    std::function<void()> rotateCounterclockwise;
    std::function<void()> requestToggleTwoPageMode;
    std::function<void()> requestToggleRightToLeftReading;
    std::function<void()> toggleInfoPanel;
    std::function<void()> toggleThumbnailPanel;
    std::function<void()> showFirstImageBoundary;
    std::function<void()> toggleFullScreen;
    std::function<void()> requestShortcutHelp;
    std::function<void()> openApplicationMenu;
    std::function<bool()> videoAvailable;
    std::function<bool()> videoSeekable;
    std::function<qint64()> videoDuration;
    std::function<void(qint64)> seekVideoBy;
    std::function<void(qint64)> setVideoPosition;
    std::function<void()> toggleVideoPlayback;
};

class ApplicationCommandRouter final
{
public:
    void handleActionTriggered(ActionId actionId, const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports) const;
    void handleScanForwardAction(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports) const;
    void handleScanBackwardAction(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports) const;
    bool executeHorizontalArrowShortcut(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports, bool leftArrow) const;
    bool executeSinglePageArrowShortcut(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports, bool leftArrow) const;
    bool executeVerticalPanShortcut(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports, bool up) const;
    bool executeVideoSeekShortcut(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports, qint64 deltaMilliseconds) const;

private:
    KiriView::ImageShortcutNavigationPolicy m_navigationPolicy;
};
}

#endif
