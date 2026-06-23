# Image Display

## Loading and Replacement

The UI remains responsive while a selected image is being opened.

If no media item is currently displayed, the UI shows a loading state.

If no media item is selected, the empty state says that no file is selected and offers Open.

If the selected image or video cannot be opened while no media item is displayed, the error state explains that the selected file could not be opened, shows the underlying error when available, and offers Open.

If a media item is already displayed and users select a different image, the new selection owns the image viewport immediately. If the new image is already available from predecode, it replaces the view immediately; otherwise KiriView clears the previous image presentation and shows the normal loading state until the new image is ready.

If a media item is already displayed and users select a video, KiriView leaves image mode immediately and shows the video loading state.

If another file is selected before the previous load finishes, only the most recent selection is displayed.

When a new image is selected while an image is already displayed and the new image is not already available from predecode, any running animation stops when the previous image presentation is cleared.

If the selected URL cannot be read or the file is not a decodable image or playable video, KiriView keeps the selected target active and shows the target's error state instead of restoring the previous media item.

If animation playback fails for the displayed image, the UI shows an error state and remains ready for another open action.

## Rendering

Opened images are displayed centered in the available page area while preserving their aspect ratio.

When KiriView is started with a direct image source, the initial image appears in the main viewport once it is display-ready. The main viewport does not require thumbnail pane visibility, information pane visibility, or another layout side effect before showing that accepted image.

Image zoom is expressed in physical display pixels. At 100%, one image pixel maps to one physical monitor pixel.

For SVG files, 100% uses the SVG's intrinsic size. SVGs remain sharp instead of pixelated when Fit mode, manual zoom, window resizing, or display scale changes the displayed size.

Static SVG rendering applies ordinary static SVG features such as clip paths. SVG script execution, animation playback, and loading external network or file resources referenced from SVG content are outside the current scope.

Static image files, including bitmap images and SVG files, appear at full resolution when they are small enough to display directly.

Large static images may first appear quickly from a trusted thumbnail preview, RAW embedded preview, or lower-detail first display. The image then becomes sharper when a matching full-detail or current-detail display is ready.

While sharper detail is being prepared for the same image, KiriView may keep the current accepted image visible. Zooming, panning, resizing, rotation, and display scale changes should not expose blank regions while a replacement for the same image is pending.

When a sharper replacement becomes available for the current image, it replaces the previous lower-detail image without changing the user's selected media target, zoom mode, or pan position except where the existing viewport rules require clamping.

SVG preview images are placeholders. KiriView may pre-render an SVG preview capped to the current physical viewport size so adjacent SVG images can appear immediately from the still-image cache. That preview may be visible while current-detail rendering is missing, but stale low-resolution SVG output does not substitute for the current-detail image after zoom, viewport, rotation, pan position, or device-pixel-ratio changes.

After zooming far out and then back in, SVG display eventually returns to current-detail rendering. Stale lower-detail SVG output does not remain visible once KiriView can provide the current-detail image.

When adjacent images are already available, Previous and Next navigation can replace the view immediately.

When ordinary direct media navigation moves from an image to a video and then back to a nearby image, previously prepared still-image data may remain available so returning to that image can avoid a full-page loading state. Direct videos themselves are not decoded into this still-image cache.

If a static image exceeds the supported decode or display size, KiriView reports an error or unsupported state for the selected target instead of restoring a previously displayed image.

HEIF-family still images, including AVIF still images, are supported when the still image is encoded with AV1, HEVC, AVC/H.264, JPEG, JPEG 2000, or VVC/H.266.

If a recognized HEIF-family still image uses an unsupported compression format or cannot be decoded, KiriView reports the decode error for the selected target instead of restoring a previously displayed image.

Camera RAW files open as static images. KiriView displays supported RAW files as display-ready 8-bit sRGB images and does not expose RAW editing controls such as demosaic, white-balance, tone-curve, or embedded-preview selection.

When KiriView recognizes selected image data as RAW, including TIFF-family RAW files such as DNG files, it reports the RAW decode result without retrying the same bytes through another image path. Ordinary TIFF files that are not recognized as RAW are decoded as general raster images.

RAW files participate in the same open, adjacent navigation, archive, and directory workflows as other supported static image files.

The image viewing area behind empty, loading, ready, and error states uses a dark background color derived from the KDE/Kirigami View color scheme. Dark color schemes use the View background color; light color schemes use the View text color so the image viewing area remains dark.

Navigation transitions do not flash to a different page color.

The toolbar keeps its normal application styling.

When an image is ready, the image viewing viewport does not add page padding around the image area.

Readiness-dependent controls, overlays, panning affordances, zoom controls, and image-only actions all use the same current media readiness state. Empty, loading, replacement, error, video, and unsupported-placeholder intervals must not expose stale ready-image affordances from the previously displayed image.

## Fit and Zoom State

KiriView starts in Fit mode.

Fit mode scales the image as large as possible while keeping the full image visible in the viewport, including upscaling small images when space is available.

Fit mode's displayed image is interpreted by the active viewport geometry frame. If a fitted axis is equal to the viewport within KiriView's geometry tolerance, that axis is not pannable and does not show a scrollbar.

Fit Height mode scales the image height to the viewport height while preserving aspect ratio.

Fit Width mode scales the image width to the viewport width while preserving aspect ratio.

Within the same directly opened archive or directory collection, KiriView preserves the current zoom state while users move between pages with Previous, Next, or page number navigation.

Switching between single-page display and Two-Page Spread preserves the user's active zoom choice. When returning from a visible two-page spread to single-page display, the current spread zoom becomes the single-page zoom.

If the user has selected Fit, Fit Height, or Fit Width, that fit mode remains selected and recalculates for each page, viewport size, and rotation change.

If the user has entered a manual zoom value, that exact percentage remains active.

When the displayed page changes inside the archive or directory collection through ordinary page navigation, any panning position from the previous page is cleared. The newly displayed page starts at its scan start at the preserved zoom level: top-left normally and top-right in Right-to-Left Reading mode.

The scan-backward shortcut may open the previous image at its final scan position instead: bottom-right normally and bottom-left in Right-to-Left Reading mode.

Starting KiriView, opening a regular image, moving between regular directory images, opening a KDE archive URL image directly, opening a different archive or directory collection, or moving to a sibling archive resets zoom to Fit mode when the new image is displayed.

## Rotation

When an image is ready and Two-Page Spread is disabled, Rotate Clockwise turns the current image view by 90 degrees clockwise and Rotate Counterclockwise turns it by 90 degrees counterclockwise.

Rotation is display-only. It does not modify image files, metadata, archive contents, remote URLs, or any saved file state.

Fit, Fit Height, Fit Width, manual zoom, scrollbars, drag panning, wheel zoom, keyboard panning, and scan shortcuts use the rotated logical image bounds.

`R` rotates clockwise, and `Shift+R` rotates counterclockwise.

Rotation resets to 0 degrees whenever a different image or page is displayed, and it also resets when the displayed image is cleared.

Rotation is unavailable while Two-Page Spread is enabled. Enabling Two-Page Spread resets rotation to 0 degrees.

## Zoom Controls

The toolbar provides a fixed-width zoom percentage input with a separate percent suffix. It shows values below 10,000% as a rounded integer percentage without digit grouping, capped at `9999 %`. It shows values from 10,000% through 999,999% in thousands using `k`, adds `+` when the actual value is above the displayed thousand bucket, and shows values at or above 1,000,000% as `999k+ %`. The editable value text is right-aligned, uses a fixed-width font, and excludes `%`; the adjacent percent suffix provides `%` after a one-space visual gap and keeps toolbar spacing before the stepper buttons.

When no image or direct video has an active zoom readout, the toolbar zoom control displays `- %`. An image zoom readout exists only while a ready image with a displayed image size is active. Empty, loading, error, and non-image document states must not expose image render-context fallback zoom values through the toolbar.

When a direct video is displayed, the toolbar zoom control remains in the same position as image mode and becomes read-only. It displays the fitted video zoom percentage when KiriView can determine the intrinsic video frame size and current displayed content size, and `? %` when the percentage is unavailable.

When an image is ready, users can enter manual zoom values from 10% through a dynamic maximum that keeps the displayed image within the application's supported display size.

The maximum manual zoom is never lower than the normal Fit zoom percentage.

Pressing Enter or clicking the image viewing area while editing the zoom input applies the nearest valid manual zoom value, returns focus to the image viewing area, and restores viewer keyboard shortcuts.

Pressing Escape while editing the zoom input cancels the edit, restores the currently applied zoom percentage in the toolbar, returns focus to the image viewing area, and does not leave fullscreen.

Editing the zoom input switches to manual zoom.

When an image is ready and the zoom input is not being edited, unmodified wheel or trackpad scrolling over the toolbar zoom percentage control switches to manual zoom and adjusts by half the normal multiplicative zoom step. One wheel detent zooms in by multiplying the current zoom by `2^(1/16)`, and one wheel detent zooms out by multiplying by `2^(-1/16)`. Toolbar wheel zoom uses the same dynamic manual zoom range as the toolbar zoom input and has no effect for read-only zoom readouts such as direct video.

The toolbar fit control is a menu button that starts with Fit to Window selected. The button displays the currently selected fit mode's icon and label and opens a menu when clicked; clicking the button itself does not apply a fit mode. The menu offers Fit to Window, Fit Width, and Fit Height; selecting a menu item immediately applies that fit mode and updates the button's icon, label, and tooltip. Manual zoom does not replace the selected fit mode displayed by the button.

When an image is ready, `=` or `+` zooms in by multiplying the current zoom by `2^(1/8)`, and `-` zooms out by multiplying the current zoom by `2^(-1/8)`.

Keyboard zoom uses the same dynamic manual zoom range as the toolbar zoom input.

Holding Ctrl and using the mouse wheel, or holding the right mouse button and using the mouse wheel, over the image viewport zooms around the cursor when the cursor is over the image, or around the nearest displayed image point when the cursor is in the viewport outside the image. Wheel up zooms in by the same multiplicative step and wheel down zooms out by its reciprocal.

Double-clicking the image viewport toggles between Fit mode and 100% manual zoom. If the current zoom mode is Fit, double-clicking switches to 100% manual zoom around the clicked viewport point when it is over the image, or around the nearest displayed image point when the click is inside the viewport outside the image. If the current zoom mode is Manual, Fit Height, or Fit Width, double-clicking switches to Fit mode.

When an image is ready, `` ` `` switches to 50% manual zoom, `1` switches to 100% manual zoom, `2` switches to 200% manual zoom, `8` selects Fit Height mode, `9` selects Fit Width mode, and `0` selects Fit to Window mode.

Viewport zoom, fit, panning, and scan results are computed from the current active presentation state. Stale or delayed viewport observations must not overwrite newer zoom, pan, rotation, resize, or device-pixel-ratio results.

During active gestures, KiriView may update the immediate visual position before the final viewport state is committed. Once a newer presentation command supersedes an older observation, the older observation must not become the authoritative viewport frame.

Switching between single-page display and Two-Page Spread is transactional. During a transition, users may temporarily see the previous committed presentation or a placeholder for the next presentation, but controls, zoom readout, panning availability, and render output must not combine properties from both presentations. If the transition cannot be completed, the previous committed presentation remains authoritative.

## Animation

Animated image files, including GIF, APNG, animated WebP, animated JPEG XL, and HEIF-family image sequences such as `.heics` and `.avifs`, play when animation frames are available.

When KiriView can identify more than one authored frame in a supported animated image file, it presents the file as an animation instead of freezing it as a static still image.

The first frame is shown once loading succeeds. Later frames use the file's frame delays and loop count.

Infinite loops continue until another image is selected or the view is cleared.

APNG animations may show the first displayable frame before all later frames are ready, so opening an APNG does not require the complete animation to be ready before the image becomes visible.

APNG animations and HEIF-family image sequences continue to use full-frame playback and play as authored.
