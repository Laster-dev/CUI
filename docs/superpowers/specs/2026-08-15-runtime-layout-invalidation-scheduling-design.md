# Runtime Layout Invalidation Scheduling Design

## Problem

Runtime layout mutations update property values and mark elements dirty, but `InvalidateMeasure()` and `InvalidateArrange()` do not request a frame from `FrameScheduler`. When the window is otherwise idle, `FlushLayout()` is not reached. This leaves StackPanel, Grid, Canvas, and other live layout examples visually unchanged until an unrelated repaint occurs.

## Scope

- Make layout invalidation schedule a window frame through `FrameScheduler`.
- Preserve dirty propagation to ancestors and avoid redundant scheduler work.
- Ensure Canvas coordinate mutations trigger arranging and repainting of both old and new footprints through the existing `SetBounds()` dirty path.
- Do not add manual page-level refresh calls.

## Behavior

1. The first `InvalidateMeasure()` or `InvalidateArrange()` in an idle tree schedules the next frame.
2. A batched series of property changes remains coalesced into one scheduled frame.
3. The scheduled frame flushes layout using the existing window render lifecycle.
4. Runtime changes to `Gap`, `Orientation`, Grid definitions, Grid attached coordinates, and Canvas coordinates update on the next frame.

## Acceptance

- StackPanel direction and gap controls visibly update immediately.
- Grid preset selection updates row/column definitions and cell positions immediately.
- Canvas random reshuffle moves the circle immediately with no residual pixels.
- CUI.Core builds in x64 Debug.
