# Extensible Thumbnail Pipeline Roadmap

This roadmap brings the thumbnail panel from placeholder-only rows to real thumbnails through an extensible source-adapter pipeline. The first shipped implementation target is direct local image thumbnails, but the runtime shape must allow direct video, archive-entry, archive-collection, and directory-collection thumbnail sources to attach later without replacing the panel, scheduler, cache lookup, result projection, or stale-completion model.

Roadmap items are intentionally sized so one agent can complete one progress item or stage at a time. Each stage that changes user-visible behavior, architecture ownership, tests, or code must follow the repository intent order in `AGENTS.md`: spec commit, architecture commit, failing-test commit when practical, implementation commit.

## Target Architecture

- The thumbnail pipeline is source-neutral: QML sends display demand, the document-session thumbnail runtime schedules work, source adapters resolve identity and renderability, cache helpers look up or install thumbnails, and the model projects ready or fallback results back to QML.
- QML stays source-agnostic: delegates compute visible thumbnail demand from their actual display area and device pixel ratio, request work through a session-owned API, and render the returned thumbnail or fallback without branching on direct image, video, archive, or collection source types.
- C++ remains the runtime owner: the document session owns QML-facing thumbnail state, async lifetime, queue priority, stale-completion rejection, and `QImage` or image-provider storage; Rust may own pure size-selection, priority, and cache-policy value logic when that removes meaningful complexity.
- `xdg-thumbnail` is the Freedesktop cache boundary: KiriView owns source acquisition and rendering, while `xdg-thumbnail` owns canonical identity helpers, validated lookup, larger-cache fallback, normalization, metadata, and atomic personal-cache installation.
- Every row is modeled through a stable `ThumbnailSourceKey` concept containing row URL, source kind, cache identity candidate, freshness facts when available, display label, navigation scope identity, and navigation generation; early stages may implement this as C++ values before adding Rust policy helpers.
- Work is modeled through a `ThumbnailDemand` concept containing source key, requested physical max edge, selected XDG size bucket, demand reason, and priority class: visible, nearby, user-navigation, or background fill.
- Completion is modeled through a `ThumbnailResult` concept that distinguishes ready image, unsupported source, missing cache pending generation, failed generation, invalid cache fallback, and stale ignored completion.

## Progress

- [x] The active navigation thumbnail strip exists as a session-owned model with icon-and-label fallback rows for the current active navigation list.
- [x] The thumbnail panel keeps the current item selected and implements intent-aware reveal behavior without generated preview thumbnails.
- [x] Focused C++ and QML integration coverage exists for the current thumbnail strip model, activation path, selection, and reveal behavior.
- [x] Declare generated preview thumbnail scope and architecture intent before implementation.
- [ ] Add source-neutral thumbnail demand reporting from QML delegates.
- [ ] Add common runtime state, source keys, result projection, and stale completion rejection.
- [ ] Add XDG cache lookup for direct local images.
- [ ] Add on-demand direct image thumbnail generation and cache installation.
- [ ] Add quality, resize, HiDPI, and memory-pressure behavior.
- [ ] Add best-state background fill scheduling and thumbnail runtime observability.
- [ ] Prove future adapter readiness for unsupported, video, archive-entry, archive-collection, and directory-collection sources.
- [ ] Satisfy the acceptance checklist for direct local image thumbnails and source-neutral extension points.

## Stage 0: Declare Intent

Goal: make the user-visible and architectural intent explicit before changing the thumbnail runtime.

Work:

- Update `docs/spec/scope.md` so generated preview thumbnails are no longer globally out of scope; state that the current supported thumbnail generation target is direct local image items in the active thumbnail strip.
- Update `docs/spec/navigation.md` so the active navigation thumbnail strip may display generated image thumbnails for supported direct local images and must keep placeholder icons for unsupported, pending, failed, direct video, and collection items.
- Update `docs/architecture/state-ownership.md` so the active navigation thumbnail strip entry names the common thumbnail runtime owner, QML demand boundary, source-adapter boundary, and source-neutral result projection.
- Add or update architecture guidance only for durable ownership and boundary rules; keep exact class names and temporary migration details in code or tests, not architecture prose.
- Commit the spec and architecture changes before implementation commits.

## Stage 1: Source-Neutral Demand Surface

Goal: let QML report thumbnail display demand without knowing which source type will produce the thumbnail.

Work:

- Add QML demand reporting from each thumbnail delegate using the actual preview image box, not the whole panel height; compute physical max edge as `ceil(max(previewWidth, previewHeight) * devicePixelRatio)`.
- Emit new demand only when source identity, selected physical bucket, visibility priority, or navigation generation changes; resize churn inside the same bucket must not reschedule work.
- Add a session-owned public method or property surface for QML to report thumbnail demand without exposing source-specific APIs to QML.
- Preserve the existing icon-and-label delegate as the fallback rendering path for missing, unsupported, failed, and not-yet-ready thumbnails.
- Add focused tests or QML checks for bucket-change-only demand behavior and fallback stability before adding cache lookup.

## Stage 2: Common Runtime State and Stale Rejection

Goal: centralize thumbnail work ownership in the document session and make every async completion safe against stale state.

Work:

- Add a C++ thumbnail runtime owned by the document session that tracks active source keys, requested buckets, row result state, active jobs, cancellation, and scope generation.
- Reject completion unless source key, selected bucket, row identity, and navigation generation still match the accepted demand.
- Project thumbnail result roles from the model with source-neutral names such as thumbnail status and thumbnail image source; do not add direct-image-only roles.
- Keep direct video and collection rows in the same model path, returning `unsupported` or fallback status rather than bypassing the pipeline.
- Add tests for stale completion rejection across URL change, bucket change, model reset, navigation generation change, and unsupported source result projection.

## Stage 3: XDG Cache Lookup for Direct Local Images

Goal: display existing Freedesktop-compatible cached thumbnails for direct local image rows without generating new thumbnails yet.

Work:

- Add `xdg-thumbnail` as a dependency and wrap its blocking lookup operations behind an async worker boundary.
- Implement a direct local image source adapter that can create a readable personal original identity from a local file path, choose the requested XDG size bucket, and call `lookup_display_thumbnail_rgba8`.
- Select the smallest XDG bucket whose max dimension is at least the requested physical max edge: `normal` for `<=128`, `large` for `<=256`, `x-large` for `<=512`, and `xx-large` above that.
- Convert validated RGBA8 lookup results into the runtime image store used by QML, preserving larger-cache fallback as a display optimization while recording the requested and source bucket for diagnostics.
- On missing or invalid cache entries, return fallback status and queue generation only after this lookup-only stage is stable.
- Add integration tests for cache hit, larger-cache fallback, missing cache fallback, invalid cache fallback, non-local URL fallback, direct video unsupported fallback, and collection unsupported fallback.

## Stage 4: On-Demand Direct Image Generation

Goal: generate and install thumbnails for supported direct local images when no usable cache entry exists.

Work:

- On cache miss for direct local image sources, render a thumbnail through KiriView-owned source loading and decoding, then install the rendered output with `xdg-thumbnail` raw install APIs.
- Render for the selected bucket max dimension, preserve source aspect ratio, apply the same orientation and static-image semantics expected for normal display thumbnails, and avoid full-size decode when an existing scaled decode path can produce the requested result.
- Write only Freedesktop personal-cache entries for readable local originals with valid freshness facts; if identity or freshness cannot be confirmed, display an in-memory thumbnail without writing a cache entry only if the adapter explicitly supports that later.
- Keep generation jobs lower priority than visible cache lookups and current navigation work; cancel or ignore jobs that no longer match active demand.
- Add tests for cache miss generation, successful install, generation failure fallback, cancellation, and stale generation completion.

## Stage 5: Quality, Resize, and HiDPI Behavior

Goal: make thumbnail display stable and correctly sized across resize, scaling, and memory-pressure scenarios.

Work:

- During panel resize, keep the currently displayed thumbnail while a newly required larger bucket is loading; replace only when the newer result arrives.
- Never upscale cached or generated thumbnails beyond their display usefulness when a larger bucket is already requested; stale smaller results may remain visible only as temporary fallback.
- Use physical display size for all bucket decisions, including fractional scale factors and movement between screens with different device pixel ratios.
- Decode or upload only thumbnails needed for current display pressure; evict in-memory thumbnail images by byte budget and priority, not by row count alone.
- Add tests for `127/128/129`, `255/256/257`, `511/512/513`, and fractional-DPR bucket boundaries, plus UI behavior when moving a window between scale factors.

## Stage 6: Best-State Background Fill

Goal: fill thumbnail cache coverage opportunistically without competing with visible work or user navigation.

Work:

- Add background fill scheduling after visible and nearby demand is satisfied, scoped to the current active navigation list and bounded by cancellation when the scope changes.
- Fill order is visible bucket first, nearby bucket second, whole-scope `normal` third, larger buckets fourth, with user navigation always able to preempt background work.
- In the ideal final state, generate each XDG standard bucket directly from the original or best available source decode rather than deriving all cache entries from `xx-large`.
- Use larger-cache fallback and materialization only as a fast display or repair path; do not make `xx-large`-only generation the steady-state strategy.
- Add observability through debug logging for demand, lookup, generation, install, rejection, and eviction decisions without exposing noisy user-facing errors for ordinary thumbnail failures.

## Stage 7: Future Adapter Readiness

Goal: keep the direct-image implementation from hard-coding assumptions that would block later video, archive, or collection adapters.

Work:

- Add compile-time or test-only fake adapters for unsupported and generated sources so the pipeline proves it is not direct-image-specific.
- Document the future direct video adapter contract: it supplies stable identity and freshness facts, extracts a representative frame, renders through the same demand/cache/result path, and keeps video playback independent.
- Document the future collection adapter contract: it supplies item identity relative to archive or directory collection scope, freshness facts for the backing collection or item when available, and renders through the same demand/cache/result path.
- If a future source cannot satisfy Freedesktop personal-cache identity or freshness requirements, keep its adapter able to return an in-memory-only result while leaving cache write disabled.
- Revisit `xdg-thumbnail` before adding non-local, archive-entry, or virtual-source cache writes; update the library when identity, freshness, or materialization belongs there rather than in KiriView.

## Acceptance Checklist

- Thumbnail panel displays real thumbnails for direct local images and placeholders for unsupported sources without layout instability.
- Bucket selection follows physical pixels, not logical panel height.
- Cache lookup, generation, install, and display reuse `xdg-thumbnail` for Freedesktop-compatible personal-cache behavior.
- The implemented runtime exposes source-neutral concepts so adding video or collection adapters does not require replacing QML delegate structure, scheduler state, stale rejection, cache lookup flow, or model result roles.
- Focused tests cover size policy, stale rejection, cache hit/miss behavior, unsupported source fallback, generation, cancellation, resize, and HiDPI behavior.
