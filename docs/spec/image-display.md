# Image Display

## Loading and Replacement

The UI remains responsive while a selected image is being opened.

If no media item is currently displayed, the UI shows a loading state.

If no media item is selected, the empty state says that no file is selected and offers Open.

If the selected image or video cannot be opened while no media item is displayed, the error state explains that the selected file could not be opened, shows the underlying error when available, and offers Open.

If a media item is already displayed, that item remains visible until the newly selected image or video is ready to replace it, with a compact loading indicator shown at the top-left of the viewing area.

If another file is selected before the previous load finishes, only the most recent selection is displayed.

When a new image is selected while an image is already displayed, any running animation keeps playing until the replacement image is ready.

If the selected URL cannot be read or the file is not a decodable image or playable video, the current media item remains visible and the app reports the error without disrupting the view.

If no media item is currently displayed, failures show an error state.

If animation playback fails for the displayed image, the UI shows an error state and remains ready for another open action.

## Rendering

Opened images are displayed centered in the available page area while preserving their aspect ratio.

Image zoom is expressed in physical display pixels. At 100%, one image pixel maps to one physical monitor pixel.

For SVG files, 100% uses the SVG's intrinsic size. SVGs remain sharp instead of pixelated when Fit mode, manual zoom, window resizing, or display scale changes the displayed size.

Static SVG rendering applies ordinary static SVG features such as clip paths. SVG script execution, animation playback, and loading external network or file resources referenced from SVG content are outside the current scope.

Static image files, including bitmap images and SVG files, appear at full resolution when they are small enough to display directly.

Larger JPEG images may first appear quickly at a lower level of detail. The visible area then becomes sharper for the current zoom, pan position, viewport size, and display scale.

Newly visible areas may briefly show lower detail until sharper data is ready, but routine panning should avoid blank regions.

Panning a tiled static image updates the current render frame as well as tile decode scheduling. Tiles that belonged only to a previous visible area must not remain composited after the viewport position changes.

For tiled static images, KiriView composites only the active tile layer for the current display scale. Cached tiles from other raster levels or SVG scale buckets may remain available for reuse, but they are not drawn over the current view.

For SVG files, the active tile layer is the selected SVG raster scale bucket rather than a bitmap pyramid level.

SVG tile decoding may use integer raster texture rectangles. Display placement preserves the exact fractional intrinsic SVG coverage represented by each bucket tile so adjacent tiles meet without visible gaps or overlaps at high zoom.

SVG preview images are placeholders. KiriView may pre-render an SVG preview capped to the current physical viewport size so adjacent SVG images can appear immediately from the still-image cache. That preview may be visible while current-detail tiles are missing, but stale low-resolution SVG bucket tiles do not substitute for the active bucket after zoom, viewport, rotation, pan position, or device-pixel-ratio changes.

After zooming far out and then back in, the visible SVG area eventually returns to current-detail rendering. Stale lower-detail tiles do not remain composited once KiriView can decide the active layer for the current view.

When adjacent images are already available, Previous and Next navigation can replace the view immediately.

When ordinary direct media navigation moves from an image to a video and then back to a nearby image, previously prepared still-image data may remain available so returning to that image can avoid a full-page loading state. Direct videos themselves are not decoded into this still-image cache.

If a static image exceeds the supported decode or display size, KiriView reports a decode error without replacing the currently displayed image.

HEIF-family still images, including AVIF still images, are supported when the still image is encoded with AV1, HEVC, AVC/H.264, JPEG, JPEG 2000, or VVC/H.266.

If a recognized HEIF-family still image uses an unsupported compression format or cannot be decoded, KiriView reports the decode error without replacing the currently displayed image.

Camera RAW files open as static images. KiriView processes supported RAW files with LibRaw into display-ready 8-bit sRGB images and does not expose RAW editing controls such as demosaic, white-balance, tone-curve, or embedded-preview selection.

When KiriView recognizes selected image data as RAW, including TIFF-family RAW files such as DNG files, it uses LibRaw for that input and reports the LibRaw decode result without retrying the same bytes through another image backend. Ordinary TIFF files that are not recognized as RAW are decoded as general raster images through Qt imageformats.

RAW files participate in the same open, adjacent navigation, archive, and directory workflows as other supported static image files.

The image viewing area behind empty, loading, ready, and error states uses a dark background color derived from the KDE/Kirigami View color scheme. Dark color schemes use the View background color; light color schemes use the View text color so the image viewing area remains dark.

Navigation transitions do not flash to a different page color.

The toolbar keeps its normal application styling.

When an image is ready, the image viewing viewport does not add page padding around the image area.

## Fit and Zoom State

KiriView starts in Fit mode.

Fit mode scales the image as large as possible while keeping the full image visible in the viewport, including upscaling small images when space is available.

Fit mode's displayed image is interpreted by the active viewport geometry frame. If a fitted axis is equal to the viewport within KiriView's geometry tolerance, that axis is not pannable and does not show a scrollbar.

Fit Height mode scales the image height to the viewport height while preserving aspect ratio.

Fit Width mode scales the image width to the viewport width while preserving aspect ratio.

Within the same directly opened archive or directory collection, KiriView preserves the current zoom state while users move between pages with Previous, Next, or page number navigation.

If the user has selected Fit, Fit Height, or Fit Width, that fit mode remains selected and recalculates for each page and viewport size.

If the user has entered a manual zoom value, that exact percentage remains active.

When the displayed page changes inside the archive or directory collection through ordinary page navigation, any panning position from the previous page is cleared. The newly displayed page starts at its scan start at the preserved zoom level: top-left normally and top-right in Right-to-Left Reading mode.

The scan-backward shortcut may open the previous image at its final scan position instead: bottom-right normally and bottom-left in Right-to-Left Reading mode.

Starting KiriView, opening a regular image, moving between regular directory images, opening a KDE archive URL image directly, opening a different archive or directory collection, or moving to a sibling archive resets zoom to Fit mode when the new image is displayed.

## Rotation

When an image is ready and Two-Page Spread is disabled, Rotate Clockwise turns the current image view by 90 degrees clockwise and Rotate Counterclockwise turns it by 90 degrees counterclockwise.

Rotation is display-only. It does not modify image files, metadata, archive contents, remote URLs, or any saved file state.

Fit, Fit Height, Fit Width, manual zoom, scrollbars, drag panning, wheel zoom, keyboard panning, and scan shortcuts use the rotated logical image bounds.

Ctrl+R rotates clockwise, and Ctrl+Shift+R rotates counterclockwise.

Rotation resets to 0 degrees whenever a different image or page is displayed, and it also resets when the displayed image is cleared.

Rotation is unavailable while Two-Page Spread is enabled. Enabling Two-Page Spread resets rotation to 0 degrees.

## Zoom Controls

The toolbar provides a fixed-width zoom percentage input. It shows values below 10,000% as a rounded integer percentage without digit grouping, capped at `9999 %`. It shows values from 10,000% through 999,999% in thousands using `k`, adds `+` when the actual value is above the displayed thousand bucket, and shows values at or above 1,000,000% as `999k+ %`. The displayed text is right-aligned, uses a fixed-width font, and includes one space between the number or suffix and `%`.

When no image or direct video has an active zoom readout, the toolbar zoom control displays `- %`. An image zoom readout exists only while a ready image with a displayed image size is active. Empty, loading, error, and non-image document states must not expose image render-context fallback zoom values through the toolbar.

When a direct video is displayed, the toolbar zoom control remains in the same position as image mode and becomes read-only. It displays the fitted video zoom percentage when KiriView can determine the intrinsic video frame size and current displayed content size, and `? %` when the percentage is unavailable.

When an image is ready, users can enter manual zoom values from 10% through a dynamic maximum that keeps the displayed image within the application's supported display size.

The maximum manual zoom is never lower than the normal Fit zoom percentage.

Pressing Enter or clicking the image viewing area while editing the zoom input applies the nearest valid manual zoom value, returns focus to the image viewing area, and restores viewer keyboard shortcuts.

Pressing Escape while editing the zoom input cancels the edit, restores the currently applied zoom percentage in the toolbar, returns focus to the image viewing area, and does not leave fullscreen.

Editing the zoom input switches to manual zoom.

When an image is ready and the zoom input is not being edited, unmodified wheel or trackpad scrolling over the toolbar zoom percentage control switches to manual zoom and adjusts by half the normal multiplicative zoom step. One wheel detent zooms in by multiplying the current zoom by `sqrt(1.1)`, and one wheel detent zooms out by multiplying by `1 / sqrt(1.1)`. Toolbar wheel zoom uses the same dynamic manual zoom range as the toolbar zoom input and has no effect for read-only zoom readouts such as direct video.

The toolbar fit control is a split button. The primary button applies the currently selected fit mode and starts with Fit to Window selected. Its adjacent menu offers Fit to Window, Fit Width, and Fit Height; selecting a menu item immediately applies that fit mode and updates the primary button's icon, label, and tooltip. Manual zoom does not replace the selected fit mode used by the primary button.

When an image is ready, Ctrl+= or Ctrl++ zooms in by multiplying the current zoom by 1.1, and Ctrl+- zooms out by multiplying the current zoom by 1/1.1.

Keyboard zoom uses the same dynamic manual zoom range as the toolbar zoom input.

Holding Ctrl and using the mouse wheel, or holding the right mouse button and using the mouse wheel, over the image viewport zooms around the cursor when the cursor is over the image, or around the nearest displayed image point when the cursor is in the viewport outside the image. Wheel up zooms in by the same multiplicative step and wheel down zooms out by its reciprocal.

Double-clicking the image viewport toggles between Fit mode and 100% manual zoom. If the current zoom mode is Fit, double-clicking switches to 100% manual zoom around the clicked viewport point when it is over the image, or around the nearest displayed image point when the click is inside the viewport outside the image. If the current zoom mode is Manual, Fit Height, or Fit Width, double-clicking switches to Fit mode.

When an image is ready, Ctrl+1 selects Fit mode, Ctrl+2 selects Fit Height mode, Ctrl+3 selects Fit Width mode, and Ctrl+0 switches to 100% manual zoom.

## Animation

Animated image files, including GIF, APNG, and HEIF-family image sequences such as `.heics` and `.avifs`, play when animation frames are available.

The first frame is shown once loading succeeds. Later frames use the file's frame delays and loop count.

Infinite loops continue until another image is selected or the view is cleared.

APNG animations may show the first displayable frame before all later frames are ready, so opening an APNG does not require the complete animation to be ready before the image becomes visible.

APNG animations and HEIF-family image sequences continue to use full-frame playback and play as authored.
