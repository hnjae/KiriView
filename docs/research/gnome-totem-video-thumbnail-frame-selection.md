---
date: 2026-06-27
---

# GNOME Totem Video Thumbnail Frame Selection

## Summary

GNOME Totem, also shipped as GNOME Videos, provides `totem-video-thumbnailer` for GNOME desktop video thumbnails.

Totem does not normally use the first decoded frame when the video has a known duration. It first tries embedded cover or preview artwork from GStreamer tags, then samples several positions in the video and accepts the first frame whose raw pixel variance is above a fixed boring-image threshold.

The default behavior is best summarized as: use embedded cover/preview artwork when available; otherwise try frames at about 33%, 67%, 10%, 90%, and 50% of the video duration, returning the first one that is not mostly solid-colored.

## Selection Pipeline

The installed freedesktop thumbnailer entry invokes `totem-video-thumbnailer -s %s %u %o`, passing the requested thumbnail size, input URI, and output path.

The thumbnailer builds a GStreamer `playbin` with fake audio and video sinks, sets the URI, starts the pipeline in `GST_STATE_PAUSED`, waits for the asynchronous state change to complete, and then installs an error handler.

Before selecting a decoded video frame, Totem checks both audio and video tags for cover imagery. The helper prefers `GST_TAG_IMAGE` samples whose `image-type` is `GST_TAG_IMAGE_TYPE_FRONT_COVER`, keeps the first undefined image as a fallback, and then falls back again to `GST_TAG_PREVIEW_IMAGE`. If a cover or preview image is found, Totem saves it immediately and exits.

If no cover image is returned, Totem requires at least one video track and queries the duration in milliseconds. If the user supplied `--time`, Totem captures that exact requested second without applying the boring-frame heuristic. Otherwise, it runs `capture_interesting_frame()`.

If duration is unknown, `capture_interesting_frame()` falls back to `capture_frame_at_time(..., 0)`, which captures the current paused frame without seeking. If duration is known, it tests candidate positions in this order: `1.0 / 3.0`, `2.0 / 3.0`, `0.1`, `0.9`, `0.5`.

Each candidate seek uses `GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT`, waits for the seek to complete, then grabs the current frame through `playbin`'s `convert-sample` signal. The requested sample format is RGB for the raw path or RGBA for the GL path, with pixel aspect ratio normalized to `1/1`. Orientation tags are applied after sample conversion.

If a candidate frame is non-null and `is_image_interesting()` returns true, Totem stops searching and uses that frame. If all earlier candidates are rejected, the loop keeps the last captured image, so the 50% candidate becomes the fallback when it can be captured.

## Boring-Frame Heuristic

Totem defines `BORING_IMAGE_VARIANCE` as `256.0`.

`is_image_interesting()` treats the pixbuf as 8-bit samples, iterates over `rowstride * height` bytes, computes the mean byte value, then computes sample variance over those bytes. A frame is interesting only when that variance is greater than `256.0`.

This heuristic mostly rejects flat black, white, or solid-color frames. It does not explicitly detect faces, motion, sharpness, captions, or semantic relevance, and it can accept noisy but meaningless frames if their byte variance is high enough.

## KiriView Implications

A Totem-like strategy would be more defensive against boring first frames than a single fixed seek percentage. It costs up to five seeks and five frame conversions, but often stops after the first interesting candidate around one third into the video.

For KiriView, the directly portable policy shape is: embedded cover or preview first; if no cover, try duration-relative candidates in Totem order; accept the first candidate whose simple pixel variance is above a threshold; fall back to the last captured candidate or to the first frame when duration is unavailable.

This differs from KDE `ffmpegthumbs`, whose ordinary fallback is a single configured seek percentage, defaulting to 20%, with its expensive multi-frame smart mode disabled.

## Sources

- GNOME Totem [upstream repository](https://gitlab.gnome.org/GNOME/totem), checked on 2026-06-27 against `master` commit `ff539f0f4743dae3d1f827e8522ff12945d6e572`.
- GNOME `totem-video-thumbnailer` [Flatpak build wrapper](https://gitlab.gnome.org/GNOME/totem-video-thumbnailer), checked on 2026-06-27 against `master` commit `6cb399918571a97591ff5a4a5a41d8e46b11027a`.
- [`totem.thumbnailer.in` freedesktop thumbnailer command](https://gitlab.gnome.org/GNOME/totem/-/blob/ff539f0f4743dae3d1f827e8522ff12945d6e572/data/totem.thumbnailer.in#L1-4).
- [`totem-video-thumbnailer` wrapper builds the executable from Totem sources](https://gitlab.gnome.org/GNOME/totem-video-thumbnailer/-/blob/6cb399918571a97591ff5a4a5a41d8e46b11027a/meson.build#L42-90).
- [`check_cover_for_stream()` and `thumb_app_check_for_cover()` embedded-cover path](https://gitlab.gnome.org/GNOME/totem/-/blob/ff539f0f4743dae3d1f827e8522ff12945d6e572/src/totem-video-thumbnailer.c#L213-245).
- [`thumb_app_setup_play()` and `thumb_app_seek()` GStreamer playbin and key-unit seek path](https://gitlab.gnome.org/GNOME/totem/-/blob/ff539f0f4743dae3d1f827e8522ff12945d6e572/src/totem-video-thumbnailer.c#L392-425).
- [`is_image_interesting()` variance threshold](https://gitlab.gnome.org/GNOME/totem/-/blob/ff539f0f4743dae3d1f827e8522ff12945d6e572/src/totem-video-thumbnailer.c#L428-461).
- [`capture_interesting_frame()` candidate order and fallback behavior](https://gitlab.gnome.org/GNOME/totem/-/blob/ff539f0f4743dae3d1f827e8522ff12945d6e572/src/totem-video-thumbnailer.c#L546-595).
- [`main()` cover check, track check, duration query, and `--time` bypass](https://gitlab.gnome.org/GNOME/totem/-/blob/ff539f0f4743dae3d1f827e8522ff12945d6e572/src/totem-video-thumbnailer.c#L685-703).
- [`totem_gst_playbin_get_frame()` sample conversion and orientation handling](https://gitlab.gnome.org/GNOME/totem/-/blob/ff539f0f4743dae3d1f827e8522ff12945d6e572/src/gst/totem-gst-pixbuf-helpers.c#L30-147).
- [`totem_gst_tag_list_get_cover()` cover and preview tag fallback](https://gitlab.gnome.org/GNOME/totem/-/blob/ff539f0f4743dae3d1f827e8522ff12945d6e572/src/gst/totem-gst-pixbuf-helpers.c#L182-243).
