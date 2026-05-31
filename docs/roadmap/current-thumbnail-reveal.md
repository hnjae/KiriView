# Current Thumbnail Reveal Roadmap

This roadmap tracks the thumbnail panel behavior for keeping the current media discoverable without disrupting user-driven thumbnail browsing.

## Target UX

- The main media view and thumbnail panel share one current media selection.
- The selected thumbnail is always visually identifiable.
- The selected thumbnail is kept visible when media changes outside direct thumbnail interaction.
- Automatic scrolling preserves the user's thumbnail browsing context whenever possible.
- Exact always-centered positioning is avoided in favor of intent-aware reveal behavior.
- Fast repeated navigation remains responsive and does not queue distracting scroll animations.

## Progress

- [x] Main media selection changes when users activate an item from the thumbnail panel.
- [x] Thumbnail selection changes when the current media changes in the main media view.
- [x] Document the first user-visible reveal behavior in `docs/spec/navigation.md` before implementation.
- [x] Add focused regression coverage for basic reveal behavior.
- [x] Implement basic current-thumbnail reveal.
- [x] Track the origin of current-media changes.
- [ ] Add safe-zone based thumbnail following.
- [ ] Add direction-aware positioning.
- [ ] Add interruptible or coalesced scroll animation.
- [ ] Add focused regression coverage before each later stage when practical, expanding coverage for origin-aware, safe-zone, direction-aware, and coalesced reveal behavior.

## Stage 1: Basic Reveal

Goal: ensure the selected thumbnail is not lost when current media changes from main-view navigation.

Behavior:

- If the selected thumbnail is already visible, leave the thumbnail panel scroll position unchanged.
- If the selected thumbnail is outside the visible viewport, use index-based containment reveal.
- If the selected thumbnail is larger than the viewport or cannot be fully contained, reveal the leading or nearest visible edge without attempting exact centering.
- Do not force recentering after direct thumbnail clicks.

Done when:

- Previous, Next, First, Last, page-number entry, and equivalent main-view navigation leave the selected thumbnail visible.
- Direct thumbnail activation does not unexpectedly move the thumbnail panel.
- Focused regression coverage verifies visible, offscreen, and direct-thumbnail activation cases.

## Stage 2: Origin-Aware Reveal

Goal: make thumbnail scrolling depend on why the current media changed.

Behavior:

- Direct thumbnail activation preserves the user's thumbnail scroll context.
- Adjacent main-view navigation is eligible for automatic reveal when the selected thumbnail would otherwise leave the visible area or preferred zone.
- Large jumps such as page-number entry, file open, First, and Last may use neutral positioning that prioritizes discoverability over preserving local browsing context.
- Programmatic or loading-related selection changes do not produce stale or duplicate scroll movement.

Done when:

- Current-media changes carry explicit reveal intent such as thumbnail activation, adjacent main-view navigation, large jump, load or open, or programmatic synchronization.
- The reveal policy is testable without relying on incidental UI timing.

## Stage 3: Safe-Zone Following

Goal: replace exact centering with stable viewport bands.

Behavior:

- Define a preferred visible zone along the thumbnail view's flicking axis, inset from the visible viewport.
- Preserve cross-axis visibility for grid-style thumbnail layouts.
- If the selected thumbnail remains inside the preferred zone, do not scroll.
- If it leaves the preferred zone during eligible main-view navigation, reveal it to the nearest stable anchor inside the zone.
- Keep the selected thumbnail visible even when the panel is resized.

Done when:

- Slow adjacent navigation feels stable.
- The thumbnail strip does not move on every selection change.
- Resizing the thumbnail panel does not leave the selected thumbnail hidden.

## Stage 4: Direction-Aware Positioning

Goal: show more thumbnails in the likely navigation direction.

Behavior:

- Forward navigation positions the selected thumbnail so more following items remain visible.
- Backward navigation positions it so more preceding items remain visible.
- Large jumps still use neutral positioning.
- Direction-aware anchoring distinguishes model direction from visual leading and trailing edges.
- Right-to-left layouts may invert visual anchors while keeping Previous and Next model semantics predictable.

Done when:

- Adjacent navigation gives useful preview context ahead of the user's movement.
- Direction-aware behavior remains predictable in both normal and right-to-left reading modes.

## Stage 5: Interruptible Animation

Goal: keep fast navigation responsive.

Behavior:

- Single navigation steps may animate briefly.
- At most one automatic reveal animation or pending reveal target exists at a time.
- Repeated key navigation, held buttons, or rapid selection changes replace older pending reveal targets with the newest target.
- Selection highlight updates immediately even when scroll settling is deferred.
- User-initiated thumbnail scrolling temporarily suppresses automatic follow movement unless the current thumbnail would otherwise be lost.

Done when:

- Holding Previous or Next does not produce queued animations.
- The panel settles cleanly after rapid navigation stops.
- Manual thumbnail browsing is not interrupted by avoidable automatic scrolling.
- The latest selected thumbnail, not an older queued target, determines the final settled position.

## Notes

- User-visible behavior belongs in `docs/spec/navigation.md` before each implementation stage changes behavior.
- This roadmap intentionally avoids committing to exact pixel offsets, animation durations, or framework-specific APIs.
- The preferred implementation should use index-based view positioning and conditional reveal policy before direct coordinate math.
- Built-in highlight-range facilities may help safe-zone behavior, but they should not force automatic scrolling for origins that are supposed to preserve thumbnail browsing context.
