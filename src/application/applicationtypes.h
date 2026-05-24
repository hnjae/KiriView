// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONTYPES_H
#define KIRIVIEW_APPLICATIONTYPES_H

#include <cstddef>

namespace KiriView::ApplicationActions {
enum class MenuPresentation {
    HamburgerMenu = 0,
    MenuBar = 1,
};

enum class ActionId {
    FileOpenAction = 0,
    FileMoveToTrashAction,
    FileDeleteAction,
    GoPreviousArchiveAction,
    GoNextArchiveAction,
    GoPreviousImageAction,
    GoNextImageAction,
    GoFirstImageAction,
    GoLastImageAction,
    ViewZoomInAction,
    ViewZoomOutAction,
    ViewFitAction,
    ViewFitHeightAction,
    ViewFitWidthAction,
    ViewActualSizeAction,
    ViewRotateClockwiseAction,
    ViewRotateCounterclockwiseAction,
    ViewToggleTwoPageModeAction,
    ViewToggleRightToLeftReadingAction,
    ViewPanTopLeftAction,
    ViewPanBottomRightAction,
    ViewScanForwardAction,
    ViewScanBackwardAction,
    WindowFullscreenAction,
    HelpShortcutsAction,
    OptionsConfigureKeybindingAction,
    OptionsShowMenubarAction,
    FileQuitAction,
    ActionCount,
};

inline constexpr std::size_t actionDefinitionCount
    = static_cast<std::size_t>(ActionId::ActionCount);
}

#endif
