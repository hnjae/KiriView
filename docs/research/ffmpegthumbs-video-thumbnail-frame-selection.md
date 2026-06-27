---
date: 2026-06-27
---

# ffmpegthumbs Video Thumbnail Frame Selection

## Summary

KDE `ffmpegthumbs` does not normally use the first decoded video frame as the representative thumbnail frame.

For the ordinary `sequenceIndex() == 0` thumbnail request, `ffmpegthumbs` first tries an embedded attached picture from the video container, then falls back to a decoded frame at the first configured seek percentage, whose default is `20`.

The current default behavior is therefore best summarized as: use an embedded cover image when available; otherwise seek to about 20% of the video's duration and thumbnail the frame reached after that seek.

## Selection Pipeline

`FFMpegThumbnailer::create()` reads the KIO thumbnail request sequence index, clamps negative values to `0`, loads `sequenceSeekPercentages`, and uses `20` as a hard fallback if the configured list is empty.

The default `ffmpegthumbsrc` setting for `sequenceSeekPercentages` is `20,35,50,65,80`, so the first non-cover sequence frame is the 20% point and later sequence thumbnails rotate through 35%, 50%, 65%, and 80%.

Before decoding a video frame, `ffmpegthumbs` scans FFmpeg streams with `AV_DISPOSITION_ATTACHED_PIC` and selects the best embedded picture. If attachment metadata contains Matroska-style names, it prefers `cover_land`, then `small_cover_land`, then `cover`, then `small_cover`; otherwise it prefers the largest attached picture by byte size.

If an embedded picture was found and the sequence index is `0`, `ffmpegthumbs` returns that image. If a higher sequence index is requested, the cover consumes the first sequence slot and the percentage-based frames shift right by one.

If no embedded picture is returned, `ffmpegthumbs` sets the thumbnail size, calls `setSeekPercentage(seekPercentages[seqIdx])`, and asks its bundled `VideoThumbnailer` to generate the thumbnail.

`VideoThumbnailer::generateThumbnail()` decodes one frame before seeking, then seeks to `duration * seekPercentage / 100` seconds unless an explicit seek time was configured or an old H.264 workaround path disables seeking. After the seek, the normal path scales the current decoded frame and writes it as the thumbnail.

The seek implementation calls `av_seek_frame()` with the requested timestamp, flushes codec buffers, then decodes packets until it gets a video frame that is also marked as a key frame, up to bounded retry counts. The selected image is therefore the decoder's frame after a percentage-time seek, constrained by FFmpeg/key-frame behavior, not an exact arbitrary frame index.

## Smart Frame Selection

The bundled `VideoThumbnailer` still contains a smart-selection mode, but `ffmpegthumbs` leaves it disabled in the normal path.

The disabled smart path decodes 25 consecutive frames after the seek point, builds RGB histograms for each frame, averages the histograms, and chooses the frame whose histogram has the smallest RMSE from that average. That chooses the most visually typical frame among the sampled run, not necessarily the brightest, sharpest, least-black, or most semantically meaningful frame.

The `ffmpegthumbs` call site comments that smart frame selection is very slow compared with fixed detection and leaves a TODO to use it only if the image is single-colored.

## KiriView Implications

For a lightweight ffmpegthumbs-like fallback, KiriView should prefer embedded video cover/thumbnail imagery first, then decode around 20% of the duration rather than using the first frame.

If KiriView later wants stronger avoidance of black title cards, fades, or blank frames, ffmpegthumbs' dormant smart mode is evidence for a bounded post-seek sampling strategy, but its own comment warns that always-on histogram sampling was considered too slow for thumbnail generation.

## Sources

- KDE `ffmpegthumbs` [upstream repository](https://invent.kde.org/multimedia/ffmpegthumbs), checked on 2026-06-27 against `master` commit `53649c27c07fa9c727001e8222e0a5084f8ad347`.
- [`ffmpegthumbnailersettings5.kcfg` default `sequenceSeekPercentages`](https://github.com/KDE/ffmpegthumbs/blob/master/ffmpegthumbnailersettings5.kcfg#L12-L15).
- [`FFMpegThumbnailer::create()` sequence handling, embedded-picture priority, cover/percentage shift, and disabled smart-selection call site](https://github.com/KDE/ffmpegthumbs/blob/master/ffmpegthumbnailer.cpp#L73-L177).
- [`VideoThumbnailer::generateThumbnail()` seek-percentage behavior and smart-selection branch](https://github.com/KDE/ffmpegthumbs/blob/master/ffmpegthumbnailer/videothumbnailer.cpp#L88-L128).
- [`VideoThumbnailer::getBestThumbnailIndex()` histogram/RMSE smart-frame scoring](https://github.com/KDE/ffmpegthumbs/blob/master/ffmpegthumbnailer/videothumbnailer.cpp#L181-L215).
- [`MovieDecoder::seek()` FFmpeg seek and key-frame decode loop](https://github.com/KDE/ffmpegthumbs/blob/master/ffmpegthumbnailer/moviedecoder.cpp#L184-L223).
