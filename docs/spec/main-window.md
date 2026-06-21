# Main Window

## Toolbar

The main window has exactly one toolbar instance.

The main window toolbar shows media controls without a page title.

The leading side of the toolbar contains Previous, the current page number, `of`, the total item count, and Next.

The toolbar page navigation readout and page-number entry use the current active navigation scope. The toolbar does not show a mixed readout from more than one scope.

When active navigation is unavailable or unknown, the toolbar page navigation readout displays `– of –` and keeps the page-number entry and navigation buttons disabled.

When the active navigation scope is a directly opened CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, RAR, or local directory collection, the trailing action toolbar contains Right-to-Left Reading, Two-Page Spread, a Fit menu button, zoom, and, when Hamburger Menu presentation is active outside fullscreen, a toolbar application menu button.

When the active navigation scope is not an opened archive or directory collection, including empty state, ordinary direct image files, direct video files, and direct KDE archive-entry URLs, the trailing action toolbar does not show Right-to-Left Reading or Two-Page Spread.

Right-to-Left Reading and Two-Page Spread visibility is determined by the active navigation scope, not by whether the current media item is an image, direct video, or unsupported-video placeholder. When a direct video is displayed in an ordinary direct media scope, Fit and zoom remain in their image-mode positions; Fit is disabled and zoom is read-only.

The trailing action toolbar shows as many trailing controls as fit and moves the rest into an overflow menu. When it runs out of horizontal space, KiriView keeps the zoom percentage visible the longest, then the Fit menu button. The Fit menu button shows its selected fit label when there is enough toolbar space and collapses to icon-only when space is constrained.

Visible trailing toolbar controls align to a common vertical center and use consistent outer spacing between adjacent top-level controls. The Fit menu button is rendered as one top-level toolbar control with the same vertical alignment and spacing as adjacent visible trailing toolbar controls.

When full trailing toolbar controls fit and Right-to-Left Reading and Two-Page Spread are visible, they are text-beside-icon buttons with the toolbar labels `Right-to-Left` and `Two-Page Spread`. If the toolbar cannot fit the text-bearing controls, KiriView may collapse them to icon-only controls or move them into overflow according to Kirigami toolbar layout behavior.

Visible text-bearing Right-to-Left Reading and Two-Page Spread toolbar buttons expose control mnemonics through the toolbar button labels. Their menu labels, tooltips, action identity, shortcut configuration, checked state, and enabled state remain unchanged.

When visible, the Right-to-Left Reading control is immediately to the left of the Two-Page Spread control. It toggles archive binding between left-to-right and right-to-left reading when that option is available.

Outside fullscreen, the toolbar uses normal application header placement, reserves layout space above the image viewing area, and remains visible even when no image or video is open.

Controls that require selected, navigable, or ready media are disabled until the corresponding program state is available.

The toolbar zoom control displays the active zoom readout for the current media state. Its editable value text and percent suffix are separate visual parts, so an empty document displays `- %` while the value input owns only `-`, and the suffix preserves normal toolbar spacing before the stepper buttons.

The toolbar updates related readiness, action availability, page navigation, zoom editability, and title-subject controls together for the current media state. It must not show stale control state from a previous image, video, or viewport after a newer state has been accepted.

The toolbar page navigation arrow buttons keep their physical affordance. The left arrow button triggers Previous in Left-to-Right Reading mode and Next in Right-to-Left Reading mode. The right arrow button triggers Next in Left-to-Right Reading mode and Previous in Right-to-Left Reading mode. Each button's tooltip and accessible text follow the action that button triggers.

The toolbar page navigation arrow buttons, page-number entry, shared Previous, Next, First, and Last actions, menus, and shortcuts all target the same active navigation scope and share the same enabled state.

Configurable application actions and their shortcuts use one shared availability decision. If an action is disabled, activating its menu item, toolbar placement, context-menu placement, or shortcut has no effect.

Configurable shortcuts have a declared activation scope. `ProgramWide` shortcuts are active throughout the KiriView window subject to the action's normal enabled state. `ViewerLocal` shortcuts are active only in viewer context after the viewer shortcut gates for the action are enabled.

Users may edit a shortcut slot's key sequence but may not change that slot's activation scope.

Program-wide configurable shortcuts appear as ordinary action shortcuts in menus, Keyboard Shortcuts configuration, and Keyboard Shortcuts help.

Viewer-local configurable shortcuts are shown in Keyboard Shortcuts help and KiriView shortcut configuration as viewer-local shortcuts. They do not appear as ordinary global action shortcuts.

Viewer-local shortcuts are inactive while text input, input-method-sensitive UI, shortcut help, modal UI, inactive windows, or other viewer-suppressed states are active.

Unmodified ASCII printable key sequences are allowed for viewer-local configurable shortcuts but disallowed for program-wide configurable shortcuts.

Viewer commands use viewer-local shortcuts by default unless the action explicitly declares a program-wide shortcut slot. KiriView does not derive runtime-only viewer aliases by dropping Ctrl from program-wide shortcuts and does not keep program-wide Ctrl fallbacks for viewer-local commands.

Toolbar controls, menus, context menus, shortcut help, and shortcut handling use the same current action availability decision. During media replacement, mode switches, deletion, modal dialogs, or focus changes, KiriView must not display or trigger action state derived from an older media item, older viewport, or older UI gate observation after a newer state has been accepted.

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

The menubar and toolbar application menu display one representative configured program-wide shortcut for actions with user-configurable program-wide shortcuts through the menu action's shortcut column.

The representative shortcut is the first of the action's current configured shortcuts that is safe to display in menus. Delete shortcuts, arrow and navigation-key shortcuts, and unmodified printable shortcuts are not menu-display safe because they can affect focused text input.

Actions without a menu-display-safe program-wide representative shortcut do not show configured shortcut text in menus.

Fixed shortcuts that users cannot configure may be shown only as display-only menu or tooltip text for the control they activate. They are not user-configurable action shortcuts.

Fixed shortcuts include arrow pan/navigation, Shift+arrow two-page stepping, video Alt+Arrow seeking, F10 for the toolbar application menu, and Ctrl+M for menubar presentation.

In the menubar, representative shortcut text is visually deemphasized from the menu item label when the item is not pressed. In the toolbar application menu, representative shortcut text is displayed separately on the trailing side of the menu item.

When the menubar or toolbar application menu is open, underlined menu access keys are activatable with either the displayed mnemonic letter alone or Alt plus that mnemonic letter.

When an access key opens a submenu, the parent menu remains open and the opened menu chain continues to accept access keys for the deepest open submenu.

Pressing and releasing Alt alone while the menubar or toolbar application menu is open keeps the menu open. Releasing Alt after an access-key interaction is not treated as a request to toggle or close the menu.

## Viewer Context Menu

Right-clicking the main media viewport opens a viewer context menu at the pointer position.

Holding the right mouse button and using the mouse wheel over an image viewport performs image zoom instead of opening the viewer context menu when the button is released.

The viewer context menu is available in image and video mode, including fullscreen.

Right-clicking the toolbar, menubar, Info Panel, or Thumbnail Panel does not open the viewer context menu.

The viewer context menu contains a concise set of viewer actions: Open, Open With, active navigation, image-only view actions such as zoom, fit, and rotation, panel toggles, and Fullscreen.

The viewer context menu uses the same internal application actions as the toolbar application menu and menubar. Action identity, text, shortcut display, enabled state, and checked state remain consistent across all menus.

Image-only actions remain visible but disabled when their normal availability rules disable them, including video mode and empty state.

The right-click gesture is a fixed mouse gesture. It is not listed in Keyboard Shortcuts configuration or Keyboard Shortcuts help.

## Startup and Input

When KiriView is launched with one or more file path or URL arguments, including from a file manager's Open With action, it processes only the first argument in the supplied order and opens it at startup.

When KiriView is launched with `--verbose` or `-v`, it enables diagnostic and performance logging for the launch while otherwise following normal GUI startup and source-opening behavior.

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

When a CBZ, CBT, CB7, CBR, ZIP, TAR, 7Z, or RAR archive collection opened by KiriView is displayed and the active page position is known, the title is the archive file name, a spaced en dash, the current primary page number, `/`, the total supported item count, a spaced em dash, and `KiriView`.

When a supported direct local image, supported direct local video, or supported image page inside a ZIP-backed archive collection appears in the thumbnail strip and provides usable thumbnail identity metadata, KiriView may show a generated preview thumbnail for that item. Direct local video thumbnails may use an embedded video cover or thumbnail image when available, falling back to a decoded video frame when no usable embedded image is available. This first-class path covers ordinary local media files, CBZ, and directly opened ZIP collections. CB7/7z and other directly opened archive-collection formats, ZIP-backed entries without usable thumbnail identity metadata, directly opened directory collections, collection-internal video placeholders, non-local direct videos, and direct archive-entry media URLs keep their normal placeholder thumbnail icons.

When a directly opened local directory collection is displayed and the active page position is known, the title is the directory name, a spaced en dash, the current primary page number, `/`, the total supported item count, a spaced em dash, and `KiriView`.

When an archive collection or directly opened local directory collection is displayed and the active page position is unknown, the title omits the page counter and uses the archive file name or directory name, a spaced em dash, and `KiriView`.

KiriView does not show file paths in the window title.

When no image, video, archive page, or directory page is displayed, the window title is `KiriView`.

## Fullscreen

Ctrl+F and F11 toggle the main window between normal windowed display and system fullscreen. The viewer-local Fullscreen shortcut is F.

Fullscreen hides the system titlebar and window decorations and shows the app toolbar as a top-attached overlay toolbar above the image viewing area without reserving layout space.

The fullscreen overlay toolbar uses the normal toolbar background and padding, attaches to the top, left, and right window edges, and does not use an outer margin, rounded floating-card background, or shadow.

The fullscreen overlay toolbar contains media controls without the toolbar application menu button.

The fullscreen overlay toolbar is shown when entering fullscreen and when the pointer enters the top reveal area near the toolbar.

Fullscreen hides the pointer when entering fullscreen. Moving the pointer shows it again, and KiriView hides it again after 1.0 seconds without pointer movement.

The fullscreen overlay toolbar hides after 1.0 seconds without pointer movement unless the user is actively interacting with toolbar controls or a toolbar input is focused. Pointer hover over the toolbar or top reveal area does not keep the toolbar visible by itself.

Leaving fullscreen restores the window's previous windowed, maximized, or minimized state and restores the normal header toolbar.

## Shortcut Help

Ctrl+? and F1 open the modal Keyboard Shortcuts help dialog. The viewer-local Keyboard Shortcuts help shortcut is ?.

The Keyboard Shortcuts help is shown as a modal dialog over the main window.

The Keyboard Shortcuts help content remains contained within the modal dialog. When shortcut rows exceed the available dialog content height, the shortcut list scrolls vertically and does not overflow the dialog window.

It lists user-configurable KiriView actions and their current configured shortcut text.

Program-wide and viewer-local configurable shortcuts are both listed. Viewer-local shortcuts are identified by scope text or grouping.

Shortcut help is grouped by app-menu category headers.

Each listed action is shown as a compact form-card delegate with the action text on the leading side and one or more rounded, fixed-width keycap badges for its configured shortcut sequences on the trailing side.

It does not list fixed shortcuts, mouse gestures, or mouse-wheel gestures.

It can be dismissed with standard dialog dismissal actions such as Enter/OK, Escape, the close button, or clicking outside the dialog.

While the shortcut help dialog is open, standard dialog dismissal actions close the dialog before any fullscreen handling.

Video seek shortcuts are fixed shortcuts and are not listed in Keyboard Shortcuts configuration or shortcut help.

## Video Playback Panel

Video mode shows a video viewport with playback controls at the bottom of the video.

The panel includes play/pause, timeline position selection and scrubbing, duration and position display, and a disabled non-interactive timeline state when the media is not seekable.

In the default video-control presentation, the panel floats inside the video viewport, uses a responsive width, keeps enough minimum width for its controls, caps at a moderate desktop width, and preserves side margins on narrow viewports.

The floating panel does not reserve page layout height.

The panel remains usable in fullscreen. Its visibility, auto-hide behavior, and compact fixed-bottom presentation follow the video playback controls contract.

## Side and Thumbnail Panels

KiriView provides an Info Panel with user-visible media information for the current media state.

The Info Panel header shows an information icon, the title `Information`, and a close button that hides the panel.

When no media item is available, the Info Panel shows an unavailable state rather than stale media details.

When a media item is available, the Info Panel shows the current file name, a summary line, General and media-specific metadata sections, a Camera section only when camera metadata rows are available, and an Advanced Metadata section only when additional parsed metadata rows are available. File names and displayed paths decode percent-encoded URL path text for user readability. The current file name, summary line, and metadata row values are rendered in a fixed-width font.

The General section shows available non-placeholder file identity rows such as type and path.

For media items displayed inside an opened archive or local directory collection, the General section path is the item path relative to the collection root.

The Image section uses the current image dimensions when they are known and omits unavailable rows.

The Video section uses the current video frame dimensions when they are known. It may also show embedded video metadata such as duration and frame size when available, and omits unavailable rows.

For images, embedded metadata is parsed from the same backend-provided bytes used for image decoding, so direct files, directory collections, and archive collections follow their owning source backend.

For direct videos, embedded metadata is parsed from the resolved playback or local file path. Collection-internal video metadata is not shown while those videos remain unsupported placeholders.

An unsupported-video placeholder keeps the current media item's video identity in the Info Panel. Its General section type is Video and its media-specific section is Video, but collection-internal video metadata rows remain omitted until collection-internal video support exists.

The Camera section shows curated embedded metadata rows only when the values are available: Camera, Taken, Location, Lens, Exposure, ISO, Focal Length, and Software. Camera combines make and model when both exist. Location is shown as coordinates only.

The Advanced Metadata section is collapsed by default and contains parsed embedded tags not already consumed by curated rows, excluding empty, binary, and unprintable values.

The Info Panel provides icon buttons to copy the current file path and open the containing folder when those operations have a valid target for the current media item.

The Info Panel content is vertically scrollable and keeps labels and values on one elided line per row.

On wide windows, the Info Panel is an inline layout-reserving right-side panel that spans the full content height below the normal toolbar outside fullscreen, or the full window content height in fullscreen.

On narrow windows, the Info Panel is a right-edge overlay drawer that does not shrink the media viewport.

The Info Panel uses the same width bounds in inline and overlay modes: minimum 16 Kirigami grid units, preferred 18 Kirigami grid units, and maximum 20 Kirigami grid units.

The Thumbnail Panel is a compact, layout-reserving bottom filmstrip in the remaining media area to the left of the Info Panel. It uses the same dark viewer surface and matching foreground colors as the media viewport rather than a light page-panel surface.

The Thumbnail Panel shows a horizontal, scrollable active-navigation strip when the active navigation list is known. Each strip item shows a generated preview thumbnail when the item is a supported direct local image, supported direct local video, or supported ZIP-backed archive image page with a ready thumbnail result, and otherwise shows a placeholder media-type icon above the existing active-navigation candidate name. The candidate name is rendered in a fixed-width font on one elided line. The horizontal scrollbar occupies a dedicated lane below the strip items and must not overlap or obscure candidate names.

The Thumbnail Panel highlights the selected active-navigation item immediately when the active navigation position changes.

The Thumbnail Panel scrolls the horizontal strip enough to keep the selected item visible. Nearby automatic scroll adjustments may use a short easing animation.

Far active-navigation jumps, synchronization while the Thumbnail Panel is hidden, and rapid repeated active-navigation changes update the strip position immediately without scroll animation.

The Thumbnail Panel does not remap vertical mouse-wheel events to horizontal movement.

The Thumbnail Panel has a subtle top separator using the viewer foreground color at reduced opacity. Strip items use compact spacing, a small corner radius, and a subtle hover fill without shadow, glow, or card treatment. The selected strip item is indicated with a 2-pixel border using the theme highlight color.

The Thumbnail Panel uses image and video icons to distinguish supported still images from supported videos.

The number of visible strip items matches the active navigation total count. When active navigation is unavailable or unknown, the strip is empty.

Activating a Thumbnail Panel strip item opens the item at that active navigation number, matching the toolbar page-number entry behavior.

When both panels are visible, the Info Panel occupies the right side for the full content height, and the Thumbnail Panel occupies only the bottom of the media area that remains to its left.

The panels are resizable with splitters. The Thumbnail Panel minimum height is tall enough to show the media-type icon, one-line candidate name, and dedicated horizontal scrollbar lane without clipping. Its default resizable height range is compact, roughly 6 to 7.5 Kirigami grid units.

I toggles the Info Panel in viewer context, and T toggles the Thumbnail Panel in viewer context.

The panel toggle shortcuts are user-configurable application action shortcuts, not fixed shortcuts.

The panel toggle actions are available from the application menu, menubar, Keyboard Shortcuts configuration, and Keyboard Shortcuts help.

The panels are closed by default. Panel open state and splitter sizes are runtime-only and are not remembered across launches.

The panels remain available in fullscreen.

The overlay Info Panel closes when the user presses Escape, clicks outside the drawer, or activates its close button.

## Menu Presentation

Ctrl+M toggles the application menu presentation between Hamburger Menu and Menubar.

This shortcut is fixed, is not user-configurable, and is not listed in keyboard shortcut configuration or shortcut help.

KiriView may show Ctrl+M as display-only shortcut text on the Show Menubar menu item or tooltip.

When Hamburger Menu presentation is active outside fullscreen, F10 opens the toolbar application menu.

This shortcut is fixed, is not user-configurable, and is not listed in keyboard shortcut configuration or shortcut help.

KiriView may show F10 as display-only shortcut text on the toolbar application menu button tooltip.

## Escape and Quit

Escape first cancels an active page number or zoom input edit.

When no toolbar input is focused and the Info Panel is open, Escape closes the Info Panel before fullscreen handling.

When no toolbar input is focused and the Info Panel is closed, Escape leaves fullscreen if the main window is fullscreen.

Outside fullscreen, Escape does not close the main window.

Ctrl+Q closes the main window as the default program-wide configurable Quit shortcut.

The viewer-local configurable Quit shortcut is `q`; it closes the main window only while viewer-local shortcuts are active.

Quit shortcuts using Ctrl, Alt, or Meta remain active while those inputs are focused.

## Configurable Shortcuts

Users can open Keyboard Shortcuts configuration to configure KiriView's keyboard shortcuts.

Changing a shortcut updates the toolbar, application menu, menubar, shortcut help, and active keyboard handling consistently.

Shortcut changes apply immediately and persist across launches.

Changing a program-wide shortcut does not create a viewer-local alias, and changing a viewer-local shortcut does not create a program-wide fallback.

Unmodified ASCII printable shortcuts are not kept as program-wide user-configurable action shortcuts.
