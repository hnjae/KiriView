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

When adjacent images are already available, Previous and Next navigation can replace the view immediately.

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

Fit Height mode scales the image height to the viewport height while preserving aspect ratio.

Fit Width mode scales the image width to the viewport width while preserving aspect ratio.

Within the same directly opened archive document, KiriView preserves the current zoom state while users move between pages with Previous, Next, or page number navigation.

If the user has selected Fit, Fit Height, or Fit Width, that fit mode remains selected and recalculates for each page and viewport size.

If the user has entered a manual zoom value, that exact percentage remains active.

When the displayed page changes inside the archive document through ordinary page navigation, any panning position from the previous page is cleared. The newly displayed page starts at its scan start at the preserved zoom level: top-left normally and top-right in Right-to-Left Reading mode.

The scan-backward shortcut may open the previous image at its final scan position instead: bottom-right normally and bottom-left in Right-to-Left Reading mode.

Starting KiriView, opening a regular image, moving between regular directory images, opening a KDE archive URL image directly, opening a different archive or directory document, or moving to a sibling archive resets zoom to Fit mode when the new image is displayed.

## Rotation

When an image is ready and Two-Page Spread is disabled, Rotate Clockwise turns the current image view by 90 degrees clockwise and Rotate Counterclockwise turns it by 90 degrees counterclockwise.

Rotation is display-only. It does not modify image files, metadata, archive contents, remote URLs, or any saved file state.

Fit, Fit Height, Fit Width, manual zoom, scrollbars, drag panning, wheel zoom, keyboard panning, and scan shortcuts use the rotated logical image bounds.

Ctrl+R rotates clockwise, and Ctrl+Shift+R rotates counterclockwise.

Rotation resets to 0 degrees whenever a different image or page is displayed, and it also resets when the displayed image is cleared.

Rotation is unavailable while Two-Page Spread is enabled. Enabling Two-Page Spread resets rotation to 0 degrees.

## Zoom Controls

The toolbar provides a zoom percentage input.

When a direct video is displayed, the toolbar zoom control remains in the same position as image mode and becomes read-only. It displays the fitted video zoom percentage when KiriView can determine the intrinsic video frame size and current displayed content size, and `--%` when the percentage is unavailable.

When an image is ready, users can enter manual zoom values from 10% through a dynamic maximum that keeps the displayed image within the application's supported display size.

The maximum manual zoom is never lower than the normal Fit zoom percentage.

Pressing Enter or clicking the image viewing area while editing the zoom input applies the nearest valid manual zoom value, returns focus to the image viewing area, and restores viewer keyboard shortcuts.

Pressing Escape while editing the zoom input cancels the edit, restores the currently applied zoom percentage in the toolbar, returns focus to the image viewing area, and does not leave fullscreen.

Editing the zoom input switches to manual zoom.

A fit menu provides Fit, Fit Height, and Fit Width actions and shows the selected fit mode. The fit action returns the image to Fit mode.

When an image is ready, Ctrl+= or Ctrl++ zooms in by multiplying the current zoom by 1.1, and Ctrl+- zooms out by multiplying the current zoom by 1/1.1.

Keyboard zoom uses the same dynamic manual zoom range as the toolbar zoom input.

Holding Ctrl and using the mouse wheel over the image zooms around the cursor. Wheel up zooms in by the same multiplicative step and wheel down zooms out by its reciprocal.

When an image is ready, Ctrl+1 selects Fit mode, Ctrl+2 selects Fit Height mode, Ctrl+3 selects Fit Width mode, and Ctrl+0 switches to 100% manual zoom.

## Animation

Animated image files, including GIF, APNG, and HEIF-family image sequences such as `.heics` and `.avifs`, play when animation frames are available.

The first frame is shown once loading succeeds. Later frames use the file's frame delays and loop count.

Infinite loops continue until another image is selected or the view is cleared.

APNG animations may show the first displayable frame before all later frames are ready, so opening an APNG does not require the complete animation to be ready before the image becomes visible.

APNG animations and HEIF-family image sequences continue to use full-frame playback and play as authored.
