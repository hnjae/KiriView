# Main Window

## Toolbar

The main window has exactly one toolbar instance.

The main window toolbar shows media controls without a page title.

The leading side of the toolbar contains Previous, the current page number, `of`, the total item count, and Next.

The toolbar page navigation readout and page-number entry use the document session's active navigation projection. The toolbar does not combine raw image-document page state with separate media-navigation state.

When the active navigation projection is unavailable or unknown, the toolbar page navigation readout displays `– of –` and keeps the page-number entry and navigation buttons disabled.

When an image from a directly opened CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, RAR, or local directory document is displayed, the trailing action toolbar contains Right-to-Left Reading, Two-Page Spread, zoom, Fit, and, when Hamburger Menu presentation is active outside fullscreen, a toolbar application menu button.

When no archive or directory document is open, including empty state, ordinary direct image files, direct video files, and direct KDE archive-entry URLs, the trailing action toolbar does not show Right-to-Left Reading or Two-Page Spread.

When a video is displayed, image-only toolbar controls such as editable zoom and Fit remain in the same positions but are disabled or read-only.

The trailing action toolbar shows as many trailing controls as fit and moves the rest into an overflow menu. When it runs out of horizontal space, KiriView keeps the zoom percentage visible the longest, then Fit.

When full trailing toolbar controls fit and Right-to-Left Reading and Two-Page Spread are visible, they are text-beside-icon buttons with the toolbar labels `Right-to-Left` and `Two-Page Spread`. If the toolbar cannot fit the text-bearing controls, KiriView may collapse them to icon-only controls or move them into overflow according to Kirigami toolbar layout behavior.

Visible text-bearing Right-to-Left Reading and Two-Page Spread toolbar buttons expose KDE/Qt control mnemonics through the toolbar button labels. Their menu labels, tooltips, action identity, shortcut configuration, checked state, and enabled state remain unchanged.

When visible, the Right-to-Left Reading control is immediately to the left of the Two-Page Spread control. It toggles archive binding between left-to-right and right-to-left reading when that option is available.

Outside fullscreen, the toolbar uses normal application header placement, reserves layout space above the image viewing area, and remains visible even when no image or video is open.

Controls that require selected, navigable, or ready media are disabled until the corresponding program state is available.

The toolbar zoom control displays the document session's active zoom readout rather than reading image or video document zoom values directly. When the document session is empty, the zoom control remains in place and displays `- %`.

The toolbar page navigation arrow buttons keep their physical affordance. The left arrow button triggers Previous in Left-to-Right Reading mode and Next in Right-to-Left Reading mode. The right arrow button triggers Next in Left-to-Right Reading mode and Previous in Right-to-Left Reading mode. Each button's tooltip and accessible text follow the action that button triggers.

The toolbar page navigation arrow buttons, page-number entry, shared Previous, Next, First, and Last actions, menus, and shortcuts dispatch through the document session's active navigation dispatch. Their enabled state comes from the same active navigation projection.

Open, Open With, Previous Archive, and Next Archive are provided by the application menu and shortcuts rather than fixed toolbar buttons. Previous Archive and Next Archive use visually distinct previous/next-use icons so they are not confused with page Previous and Next navigation.

## Application Menu and Menubar

KiriView shows its application menu through a toolbar application menu button by default outside fullscreen.

Users can switch the application menu presentation between Hamburger Menu and a conventional menubar. KiriView remembers the selected presentation as application state across launches.

In Hamburger Menu mode outside fullscreen, the toolbar application menu contains application actions such as Open, Open With, archive navigation, Full Screen, shortcut configuration, shortcut help, and Quit.

In menubar mode outside fullscreen, those application actions are available from the menubar. Any toolbar overflow menu appears only when toolbar controls do not fit.

The conventional menubar is an in-window menubar. Native or global menubar integration is outside the current scope.

In fullscreen, KiriView hides both the menubar and toolbar application menu button. Actions with configured shortcuts remain available through those shortcuts.

The menubar and toolbar application menu keep each action's identity, text, shortcut, and enabled state unchanged.

The Open With action is labeled `Open With...` in menus and delegates application selection and launching to KDE/KIO open-with handling. It is placed immediately after Open in the File menu and toolbar application menu.

Adjacent navigation actions are projected in reading progression order. When Right-to-Left Reading is active, the adjacent page navigation pair is displayed as Next before Previous, and the adjacent archive navigation pair is displayed as Next Archive before Previous Archive. First and Last keep their normal order because their page-index meaning does not change with reading direction.

The menubar Go menu projects directional navigation icons to match the displayed reading progression meaning without changing the underlying action identity. When Right-to-Left Reading is active, Next uses the previous-direction icon, Previous uses the next-direction icon, First uses the last-boundary icon, and Last uses the first-boundary icon.

The toolbar application menu is a single popup menu surface. Activating the toolbar application menu button and pressing F10 open a toolbar application menu with the same width, actions, access keys, and shortcut column.

Activating the toolbar application menu button while that menu is open closes it. Pressing F10 opens the toolbar application menu and leaves it open when it is already open.

The menubar and toolbar application menu display one representative configured shortcut for actions with user-configurable shortcuts through the menu action's shortcut column.

The representative shortcut is the first of the action's current configured shortcuts that is safe to display in menus. Delete shortcuts, arrow and navigation-key shortcuts, and unmodified printable shortcuts are not menu-display safe because they can affect focused text input.

Actions without a menu-display-safe representative shortcut do not show configured shortcut text in menus.

Fixed shortcuts that users cannot configure may be shown only as display-only menu or tooltip text for the control they activate. They are not user-configurable action shortcuts.

In the menubar, representative shortcut text is visually deemphasized from the menu item label when the item is not pressed. In the toolbar application menu, representative shortcut text is displayed separately on the trailing side of the menu item.

When the menubar or toolbar application menu is open, underlined menu access keys are activatable with either the displayed mnemonic letter alone or Alt plus that mnemonic letter.

When an access key opens a submenu, the parent menu remains open and the opened menu chain continues to accept access keys for the deepest open submenu.

Pressing and releasing Alt alone while the menubar or toolbar application menu is open keeps the menu open. Releasing Alt after an access-key interaction is not treated as a request to toggle or close the menu.

## Viewer Context Menu

Right-clicking the main media viewport opens a viewer context menu at the pointer position.

Holding the right mouse button and using the mouse wheel over an image viewport performs image zoom instead of opening the viewer context menu when the button is released.

The viewer context menu is available in image and video mode, including fullscreen.

Right-clicking the toolbar, menubar, Info Panel, or Thumbnail Panel does not open the viewer context menu.

The viewer context menu contains a concise set of viewer actions: Open, Open With, media navigation, image-only view actions such as zoom, fit, and rotation, panel toggles, and Fullscreen.

The viewer context menu uses the same internal application actions as the toolbar application menu and menubar. Action identity, text, shortcut display, enabled state, and checked state remain consistent across all menus.

Image-only actions remain visible but disabled when their normal availability rules disable them, including video mode and empty state.

The right-click gesture is a fixed mouse gesture. It is not listed in Keyboard Shortcuts configuration or Keyboard Shortcuts help.

## Startup and Input

When KiriView is launched with one or more file path or URL arguments, including from a file manager's Open With action, it processes only the first argument in the supplied order and opens it at startup.

Activating the open action shows the XDG portal file chooser, which accepts a single selection only.

Dropping one or more file or URL items onto the running main window opens only the first item in the order supplied by the desktop environment.

If the first startup argument is a local file path or file URL and that file does not exist, KiriView prints a clear error message naming the path and reason to standard error, does not open the main window, and exits with code 2.

Startup arguments, drop, and open dialog selection may open supported direct video files.

## Window Size and Title

KiriView permits compact window sizes down to 14 by 12 Kirigami grid units.

When no saved window geometry overrides the launch size, KiriView opens at 24 by 20 Kirigami grid units.

When a direct image file is displayed and its intrinsic image size is known, the main window title is the displayed image file name, a spaced en dash, the intrinsic size as `width×height` using the multiplication sign with no spaces, a spaced em dash, and `KiriView`.

When a direct image file is displayed and its intrinsic image size is unknown, the title omits the size and uses the displayed image file name, a spaced em dash, and `KiriView`.

When a direct video file is displayed and its intrinsic video frame size is known, the main window title is the original direct media URL's file name, a spaced en dash, the intrinsic size as `width×height` using the multiplication sign with no spaces, a spaced em dash, and `KiriView`.

When a direct video file is displayed and its intrinsic video frame size is unknown, the title omits the size and uses the original direct media URL's file name, a spaced em dash, and `KiriView`.

When a CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, or RAR archive opened by KiriView is displayed and the active document page position is known, the title is the archive file name, a spaced en dash, the current primary page number, `/`, the total supported item count, a spaced em dash, and `KiriView`.

When a directly opened local directory is displayed and the active document page position is known, the title is the directory name, a spaced en dash, the current primary page number, `/`, the total supported item count, a spaced em dash, and `KiriView`.

When an archive or directly opened local directory is displayed and the active document page position is unknown, the title omits the page counter and uses the archive file name or directory name, a spaced em dash, and `KiriView`.

KiriView does not show file paths in the window title.

When no image, video, archive page, or directory page is displayed, the window title is `KiriView`.

## Fullscreen

Ctrl+F and F11 toggle the main window between normal windowed display and system fullscreen.

Fullscreen hides the system titlebar and window decorations and shows the app toolbar as a top-attached overlay toolbar above the image viewing area without reserving layout space.

The fullscreen overlay toolbar uses the normal toolbar background and padding, attaches to the top, left, and right window edges, and does not use an outer margin, rounded floating-card background, or shadow.

The fullscreen overlay toolbar contains media controls without the toolbar application menu button.

The fullscreen overlay toolbar is shown when entering fullscreen and when the pointer moves over the window.

It hides after 1.0 seconds of inactivity unless the pointer is over the toolbar or a toolbar input is focused.

Leaving fullscreen restores the window's previous windowed, maximized, or minimized state and restores the normal header toolbar.

## Shortcut Help

Ctrl+? and F1 open the modal Keyboard Shortcuts help dialog.

The Keyboard Shortcuts help is shown as a modal dialog over the main window.

It lists user-configurable KiriView actions and their current configured shortcut text.

Shortcut help is presented as a Kirigami form-style two-column list with the shortcut text as the leading label and the action text as the value.

It does not list fixed shortcuts, mouse gestures, or mouse-wheel gestures.

It can be dismissed with standard dialog dismissal actions such as Enter/OK, Escape, the close button, or clicking outside the dialog.

While the shortcut help dialog is open, standard dialog dismissal actions close the dialog before any fullscreen handling.

Video seek shortcuts are fixed shortcuts and are not listed in Keyboard Shortcuts configuration or shortcut help.

## Video Playback Panel

Video mode shows a video viewport with a Kirigami floating playback panel over the bottom of the video.

The panel includes play/pause, timeline position selection and scrubbing, duration and position display, and a disabled non-interactive timeline state when the media is not seekable.

The floating panel uses a responsive width based on the video viewport, targets 65% of that width, keeps enough minimum width for its controls, caps at a moderate desktop width, and preserves side margins on narrow viewports.

The floating panel does not reserve page layout height.

The floating panel remains usable in fullscreen and remains visible while video mode is active in the MVP.

## Side and Thumbnail Panels

KiriView provides empty shell panels for future media information and thumbnail navigation content.

The Info Panel is a layout-reserving right-side panel that spans the full content height below the normal toolbar outside fullscreen, or the full window content height in fullscreen.

The Thumbnail Panel is a layout-reserving bottom panel in the remaining media area to the left of the Info Panel.

When both panels are visible, the Info Panel occupies the right side for the full content height, and the Thumbnail Panel occupies only the bottom of the media area that remains to its left.

The panels are resizable with splitters.

Ctrl+I toggles the Info Panel, and Ctrl+T toggles the Thumbnail Panel.

The panel toggle shortcuts are user-configurable application action shortcuts, not fixed shortcuts.

The panel toggle actions are available from the application menu, menubar, Keyboard Shortcuts configuration, and Keyboard Shortcuts help.

The panels are closed by default. Panel open state and splitter sizes are runtime-only and are not remembered across launches.

The panels remain available in fullscreen.

## Menu Presentation

Ctrl+M toggles the application menu presentation between Hamburger Menu and Menubar.

This shortcut is fixed, is not user-configurable, and is not listed in keyboard shortcut configuration or shortcut help.

KiriView may show Ctrl+M as display-only shortcut text on the Show Menubar menu item or tooltip.

When Hamburger Menu presentation is active outside fullscreen, F10 opens the toolbar application menu.

This shortcut is fixed, is not user-configurable, and is not listed in keyboard shortcut configuration or shortcut help.

KiriView may show F10 as display-only shortcut text on the toolbar application menu button tooltip.

## Escape and Quit

Escape first cancels an active page number or zoom input edit.

When no toolbar input is focused, Escape leaves fullscreen if the main window is fullscreen.

Outside fullscreen, Escape does not close the main window.

Ctrl+Q closes the main window.

Quit shortcuts using Ctrl, Alt, or Meta remain active while those inputs are focused.

## Configurable Shortcuts

Users can open Keyboard Shortcuts configuration to configure KiriView's keyboard shortcuts.

Changing a shortcut updates the toolbar, application menu, menubar, shortcut help, and active keyboard handling consistently.

Shortcut changes apply immediately and persist across launches.

When an action has a user-configurable shortcut that uses Ctrl or Ctrl+Shift and can be used safely as a viewer-only shortcut without Ctrl, KiriView derives a matching runtime-only viewer alias by dropping Ctrl from that shortcut. Examples include Ctrl+L to `l`, Ctrl+Shift+R to Shift+R, Ctrl+< to `<`, Ctrl+[ to `[`, Ctrl+Home to Home, and Ctrl+End to End. Derived aliases are not stored, are inactive while the page number or zoom input is focused, are not user-configurable shortcuts, and are not displayed as separate shortcuts in menus, Keyboard Shortcuts configuration, or the Keyboard Shortcuts help.

Unmodified ASCII printable shortcuts are not kept as user-configurable action shortcuts.
