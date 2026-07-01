# Comic Archives

## Two-Page Spread and Reading Direction

When the active navigation scope is a directly opened local CBZ, CBT, CB7, or CBR comic book archive, `S` toggles Two-Page Spread.

Two-Page Spread displays the current page on the left and the next page on the right when both pages are eligible images.

The first archive page is treated as a cover and is always displayed alone.

Any page whose pixel width is greater than its pixel height is treated as a wide page and is displayed alone.

If the next page after the current page is wide, the current page is displayed alone and the next navigation action opens that wide page.

If the next opened collection item after the current image is a video, the current image is displayed alone and the next navigation action opens that video item. Eligible stored ZIP and plain TAR video entries play in video mode; ineligible video entries show the unsupported-video placeholder.

Two-Page Spread is unavailable for ordinary image files, direct video files, KDE-supported archive URLs, directly opened ZIP, TAR, 7Z, or RAR archives, and directly opened directories.

Showing a disabled Two-Page Spread toolbar control while a directly opened ZIP, TAR, 7Z, RAR, or directory collection is active does not make Two-Page Spread available.

When the active navigation scope is a directly opened local CBZ, CBT, CB7, or CBR comic book archive, `B` toggles Right-to-Left Reading mode.

Right-to-Left Reading mode is off by default, is unavailable for ordinary image files, direct video files, KDE-supported archive URLs, directly opened ZIP, TAR, 7Z, or RAR archives, and directly opened directories, and is not saved as a global setting.

Showing a disabled Right-to-Left Reading toolbar control while a directly opened ZIP, TAR, 7Z, RAR, or directory collection is active does not make Right-to-Left Reading available.

Moving to a sibling comic book archive with Previous Archive or Next Archive preserves the current Right-to-Left Reading mode state.

When Two-Page Spread shows two pages, zooming and panning operate on the combined two-page spread as one virtual image.

Fit, Fit Height, Fit Width, manual zoom, scrollbars, drag panning, wheel zoom, keyboard panning, and scan shortcuts use the full spread bounds.

The spread has no added page gap.

The page number, window title, deletion target, archive navigation position, and one-page movement start from the primary/current page.

In Left-to-Right Reading mode, the primary/current page is rendered on the left and the next page is rendered on the right.

In Right-to-Left Reading mode, the primary/current page is rendered on the right and the next page is rendered on the left.

When navigation in Two-Page Spread targets another eligible two-page spread, KiriView shows the loading state instead of leaving the previous spread visible or showing only the left target page.

The target spread appears only after both pages are ready.

If the target page is the cover, wide, last page, or cannot be paired with an eligible next page, KiriView displays the target page alone once that page is ready.

While a Two-Page Spread transition is loading, the previously displayed page or spread remains the last committed image presentation. KiriView must not expose a partially prepared target spread as the active image presentation.

## Two-Page Spread Navigation

When Two-Page Spread is enabled for a directly opened comic book archive, ordinary Previous and Next page navigation move by the currently displayed spread rather than always by one page.

If two pages are visible, Next opens the first page after the displayed spread in reading order.

If only one page is visible, Next opens the next page.

Previous opens the previous eligible spread in reading order, falling back to the immediately previous page in that order when that page is the cover or a wide page.

Shift+Left and Shift+Right are fixed viewer-only shortcuts for moving by exactly one page while an image is ready.

Their direction is physical: in Left-to-Right Reading mode, Shift+Left opens the previous page and Shift+Right opens the next page; in Right-to-Left Reading mode, Shift+Left opens the next page and Shift+Right opens the previous page.

These fixed shortcuts are inactive while the page number or zoom input is focused or Keyboard Shortcuts help is open, and are not user-configurable actions.

When a Two-Page Spread navigation target is loading and the supported media list is known, additional Previous, Next, Shift+Left, or Shift+Right navigation is calculated from the most recent requested primary page.

KiriView moves by one primary page selection at a time while loading. The final single-page or two-page spread pairing is decided after the selected primary page has loaded.

## Archive Navigation

An archive for archive navigation is a local CBZ, CBT, CB7, or CBR comic book archive.

The current archive can be the comic book archive whose image is displayed, or an empty sibling comic book archive reached through archive navigation.

When the current media item is inside a local comic book archive opened directly by KiriView, its archive is that archive file.

The Previous Archive and Next Archive actions open the previous or next sibling comic book archive beside the current archive.

When the current item is a normal image file, direct video file, inside a KDE-supported archive URL, or inside a directly opened ZIP, TAR, 7Z, or RAR archive collection, the Previous Archive and Next Archive actions are disabled.

Previous Archive and Next Archive are also disabled inside directly opened directory collections.

Supported video entries inside a comic book archive opened directly by KiriView participate in archive collection navigation. Eligible stored ZIP and plain TAR video entries play in video mode; ineligible video entries show unsupported-video placeholders.

KDE archive URLs that point at individual video file entries remain direct media URLs and do not create comic book archive collection context.

Sibling archive candidates are local `.cbz`, `.cbt`, `.cb7`, or `.cbr` files in the current archive's parent directory.

Candidates are sorted with the same user locale-aware file name order used for image navigation.

Navigation does not wrap. Pressing Previous Archive on the first candidate keeps the current view unchanged and shows `No previous collection`; pressing Next Archive on the last candidate keeps the current view unchanged and shows `No next collection`.

`[` opens the previous sibling archive and `]` opens the next sibling archive when archive navigation is available.

`Home` opens the first supported media item in the current archive, and `End` opens the last supported media item in the current archive.

Opening a comic book archive displays the first supported media item in that archive using the same archive media ordering as page navigation. If the first supported item is a playable collection video, KiriView plays it. If the first supported item is an ineligible video, KiriView displays the unsupported-video placeholder.

When KiriView is started with a direct comic book archive source, the first supported media item appears in the main viewport once it is display-ready or represented by an unsupported-video placeholder. Thumbnail pane visibility, information pane visibility, and other layout side effects do not control whether the accepted archive item is rendered.

If a target sibling archive has no supported media, KiriView clears any displayed image and shows an error state explaining that the selected collection does not contain any supported media.

That empty archive remains the current archive navigation position, so Previous Archive and Next Archive can continue to move to neighboring archives.
