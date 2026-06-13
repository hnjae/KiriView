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
    FileOpenWithAction,
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
    ViewZoom50PercentAction,
    ViewZoom100PercentAction,
    ViewZoom200PercentAction,
    ViewFitAction,
    ViewFitHeightAction,
    ViewFitWidthAction,
    ViewRotateClockwiseAction,
    ViewRotateCounterclockwiseAction,
    ViewToggleTwoPageModeAction,
    ViewToggleRightToLeftReadingAction,
    ViewToggleInfoPanelAction,
    ViewToggleThumbnailPanelAction,
    ViewGoToContentStartAction,
    ViewGoToContentEndAction,
    ViewScanForwardAction,
    ViewScanBackwardAction,
    ViewToggleVideoPlaybackAction,
    WindowFullscreenAction,
    HelpShortcutsAction,
    OptionsConfigureKeybindingAction,
    OptionsShowMenubarAction,
    OpenApplicationMenuAction,
    FileQuitAction,
    ActionCount,
};

inline constexpr std::size_t actionDefinitionCount
    = static_cast<std::size_t>(ActionId::ActionCount);

enum class ApplicationShortcutActivationScope {
    ProgramWide = 0,
    ViewerLocal,
};

enum class ImageShortcutScope {
    HelpShortcutScope = 0,
    ViewerShortcutScope,
    ReadyShortcutScope,
    ReadyViewerShortcutScope,
    ImageSelectionShortcutScope,
    ImageSelectionViewerShortcutScope,
    PageShortcutScope,
    PageViewerShortcutScope,
    RightToLeftReadingShortcutScope,
    RightToLeftReadingViewerShortcutScope,
    RotateShortcutScope,
    RotateViewerShortcutScope,
    PannableShortcutScope,
    PannableViewerShortcutScope,
    ContainerShortcutScope,
    ContainerViewerShortcutScope,
    MediaStartEndViewerShortcutScope,
};

struct ShortcutRouteSpec {
    ApplicationShortcutActivationScope activationScope
        = ApplicationShortcutActivationScope::ProgramWide;
    ImageShortcutScope shortcutScope = ImageShortcutScope::HelpShortcutScope;
};

inline constexpr std::size_t maxShortcutRouteSpecCount = 3;
}

#endif
