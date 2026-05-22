# Product Specifications

This directory is the canonical source for KiriView's user-visible behavior. The README is an index only; the subject files below own the actual specifications.

Before implementing a product behavior change, update every relevant subject file with the intended behavior, commit that spec update, then implement against the committed spec. Development-only and repository-maintenance changes do not require a spec update.

- [Scope](scope.md): product boundaries, supported top-level content types, localization baseline, and explicit out-of-scope areas.
- [File Access](file-access.md): supported sources, Flatpak access, deletion behavior, and live directory updates.
- [Main Window](main-window.md): toolbar, menus, startup, window state, fullscreen, shortcut help, video controls, and quit behavior.
- [Image Display](image-display.md): loading and replacement, rendering, fit and zoom state, rotation, zoom controls, and animation.
- [Navigation](navigation.md): page controls, adjacent media, sorting, boundary feedback, panning, seeking shortcuts, scan shortcuts, and background loading.
- [Comic Archives](comic-archives.md): two-page spread, reading direction, archive page navigation, and archive ordering behavior.
- [Video Playback](video-playback.md): direct video scope, source identity, playback, video navigation, seeking, and deletion behavior.
