# CUI Rendering and Animation Overhaul Task

## Background

The current CUI rendering path is functional but not yet suitable for consistently smooth, high-refresh animation under complex UI load. Symptoms already observed:

- animation stutter
- low apparent frame rate
- synchronous repaint pressure during active animation
- excessive full-tree redraw work
- partial-repaint instability when no retained render cache is present

The target is to upgrade the framework from a mostly immediate-mode redraw model to a mature retained rendering pipeline with robust animation scheduling, dirty-region tracking, layer caching, and safe composition.

This task is not a cosmetic optimization pass. It is a rendering architecture upgrade.

## Primary Goal

Deliver a production-grade rendering and animation system for CUI that:

- feels consistently smooth during scroll, hover, press, ripple, toast, tab, dialog, and input animations
- avoids flicker, tearing, stale pixels, and missing-region artifacts
- scales to more complex control trees without falling back to full-window work every frame
- remains debuggable, measurable, and safe to roll back per subsystem

## Non-Goals

This task does not aim to:

- promise zero defects before implementation and validation
- rewrite unrelated business logic or control APIs unless required by rendering architecture
- add speculative micro-optimizations before the retained pipeline exists

## Problem Summary

Current bottlenecks and structural issues:

1. Window animation frames still depend too much on invalidation and repaint cadence.
2. Rendering is effectively full-scene redraw oriented.
3. Dirty-region logic is too coarse for true partial composition.
4. There is no retained render cache layer for safe dirty-region reuse.
5. Many controls still compute animation progress directly in control code.
6. Scroll-heavy controls are not yet backed by composited scroll layers and virtualization.
7. There is no first-class render diagnostics layer for cache hit rate, dirty rectangles, or dropped-frame tracking.

## Target Architecture

### 1. Retained Rendering Model

Introduce a retained rendering pipeline where visual content can be cached and recomposited without forcing every control to repaint every frame.

Core ideas:

- stable render-node representation per visible element
- cached offscreen content per eligible node/layer
- explicit dirty propagation from leaf to root
- root composition pass into the swap chain target

### 2. Dirty Region System

Introduce a dedicated dirty-region manager that supports:

- local dirty rect
- world dirty rect
- rect list merging
- old-bounds + new-bounds invalidation
- degeneration to full-window repaint when dirty complexity exceeds threshold

This must replace ad hoc single-rect union logic for high-motion scenes.

### 3. Render Layer Cache

Add a render cache layer for controls and containers whose content can be reused between frames.

Each cacheable layer should track:

- cache validity
- content dirty
- transform dirty
- clip dirty
- opacity dirty
- size dirty
- last render bounds

A node must be able to reuse cached content when:

- internal content did not change
- only transform or translation changed
- parent composition changed but node raster content remained valid

### 4. Animation Manager

Introduce a unified animation manager responsible for:

- frame clock
- delta time clamping
- coalesced frame requests
- retargetable animations
- active animation registry
- animation completion cleanup
- future support for per-animation priority or frame throttling policies

Controls should stop acting as isolated timing islands wherever practical.

### 5. Composition Pass

Rendering should be split conceptually into:

1. update phase
2. dirty propagation phase
3. cache rebuild phase
4. composition phase
5. present phase

The window should not depend on preserving swap-chain backbuffer history implicitly. Composition must be correct even when backbuffer contents cannot be trusted between presents.

### 6. Scroll Composition

`ScrollViewer`, `ListView`, `ListBox`, and `TreeView` must move toward layer-based scrolling:

- content rendered into a composited scroll layer
- scrolling applies translation first
- newly exposed regions are patched incrementally
- large item collections use visible-range virtualization

### 7. Diagnostics and Safety

Add rendering diagnostics and subsystem kill switches.

Required diagnostics:

- dirty rect overlay
- render layer bounds overlay
- cache hit/miss counters
- layout pass counters
- rasterized node counters
- frame time stats
- dropped frame stats

Required runtime fallbacks:

- disable retained caches globally
- disable partial composition globally
- disable cache for a control type
- force full repaint

## Proposed Modules

Add or refactor toward the following modules:

- `ui/framework/render/DirtyRegion.h`
- `ui/framework/render/DirtyRegion.cpp`
- `ui/framework/render/RenderNode.h`
- `ui/framework/render/RenderNode.cpp`
- `ui/framework/render/RenderLayer.h`
- `ui/framework/render/RenderLayer.cpp`
- `ui/framework/render/CompositionContext.h`
- `ui/framework/render/CompositionContext.cpp`
- `ui/framework/animation/AnimationManager.h`
- `ui/framework/animation/AnimationManager.cpp`
- `ui/framework/text/TextLayoutCache.h`
- `ui/framework/text/TextLayoutCache.cpp`

Expected existing touch points:

- `ui/framework/window/Window.h`
- `ui/framework/window/Window.cpp`
- `ui/framework/render/GraphicsContext.h`
- `ui/framework/render/GraphicsContext.cpp`
- `ui/framework/controls/UIElement.h`
- `ui/framework/controls/UIElement.cpp`
- `ui/framework/controls/ScrollViewer.cpp`
- `ui/framework/controls/ListView.cpp`
- `ui/framework/controls/ListBox.cpp`
- `ui/framework/controls/TreeView.cpp`
- `ui/framework/controls/TabView.cpp`
- `ui/framework/controls/Toast.cpp`
- `ui/framework/controls/TextBox.cpp`

## Implementation Phases

### Phase 0: Stabilize Current Behavior

Objective:

- keep current animation improvements that are safe
- avoid all known flicker regressions
- preserve correctness before large refactor

Deliverables:

- no synchronous `UpdateWindow()`-driven animation loop
- stable delta-time-driven control animation updates
- no partial-paint skipping without retained cache support

Exit criteria:

- no flicker during hover, ripple, scroll, toast, tab, dialog

### Phase 1: Core Rendering Infrastructure

Objective:

- add retained rendering primitives without changing all controls at once

Deliverables:

- `DirtyRegion`
- `RenderNode`
- `RenderLayer`
- `CompositionContext`
- `AnimationManager`

Exit criteria:

- framework builds cleanly
- root can track dirty regions and layer invalidation
- debug overlays can be toggled

### Phase 2: Root Composition Integration

Objective:

- integrate retained composition into `Window` and `UIElement`

Deliverables:

- root composition path
- cache rebuild scheduling
- old/new bounds invalidation
- layer composition into final target

Exit criteria:

- no stale pixels
- no flicker during animated movement
- full repaint fallback still works

### Phase 3: High-Value Control Adoption

Priority controls:

1. `ScrollViewer`
2. `Toast` / `ToastCenter`
3. `TabView`
4. `TextBox`

Objective:

- move the most visible animated controls to the retained path first

Deliverables:

- composited scroll content
- cached toast body rendering
- cached tab header visuals
- stable input animation under load

Exit criteria:

- noticeably smoother real-world interaction in showcase pages

### Phase 4: Virtualized Collection Controls

Priority controls:

1. `ListView`
2. `ListBox`
3. `TreeView`

Objective:

- reduce CPU and raster pressure for item-heavy scenes

Deliverables:

- visible-range virtualization
- incremental scroll patching
- item reuse strategy where applicable

Exit criteria:

- large lists remain responsive during wheel scroll and selection changes

### Phase 5: Text and Geometry Caching

Objective:

- reduce repeated text layout and geometry recreation costs

Deliverables:

- text layout cache
- metrics cache
- common geometry cache

Exit criteria:

- repeated text-heavy frames show lower CPU cost

### Phase 6: Hardening and Verification

Objective:

- turn architecture into a maintainable subsystem

Deliverables:

- diagnostics panel or debug toggles
- stress scenes
- regression checklist
- subsystem fallback switches

Exit criteria:

- system survives resize, DPI change, transparent mode, maximize/restore, rapid input, and animation overlap

## Technical Requirements

### Dirty Region Rules

- use rect lists, not only one union rect
- invalidate old and new bounds on movement
- clip dirty propagation correctly through parent clips
- collapse to full repaint when dirty complexity becomes counterproductive

### Cache Rules

- cached content must be invalidated on size, clip, opacity, transform, or content changes as appropriate
- cache ownership and lifetime must be explicit
- cache recreation must be bounded and measurable
- caches must never show stale content after content mutation

### Animation Rules

- animation timing must be delta-time-driven
- frame spikes must be clamped
- retargeting must not cause snapping or overshoot glitches unless explicitly intended
- animations that affect only transform/opacity should avoid layout invalidation

### Window/Present Rules

- do not rely on implicit previous-frame backbuffer preservation
- composition must remain correct with flip-model presentation
- resizing and DPI changes must recreate dependent surfaces safely

### Debuggability Rules

- every retained cache should be inspectable in debug mode
- every dirty submission should be measurable
- full repaint fallback must remain one switch away

## Risks

High-risk areas:

- stale cache invalidation bugs
- old-bounds/new-bounds dirty mistakes
- clipping bugs in nested containers
- DPI/scaling mismatches in offscreen caches
- memory growth from uncontrolled layer caching
- scroll layer and hit-testing divergence
- transparent-window behavior under composition

Mitigations:

- phase rollout
- per-control opt-in before broad enablement
- debug overlays
- aggressive assertions in debug builds
- subsystem fallback flags

## Acceptance Criteria

The task is complete only when all of the following are true:

- no visible flicker in common showcase interactions
- smooth scroll feels stable under repeated wheel input
- ripple, hover, tab, toast, and dialog animation remain smooth under concurrent load
- item-heavy controls avoid excessive repaint cost
- dirty region and cache behavior are observable in debug mode
- full repaint fallback remains available
- build passes cleanly
- showcase remains behaviorally correct after resize, DPI change, maximize/restore, and transparent mode scenarios

## Suggested Verification Matrix

Manual verification scenarios:

1. Rapid wheel scroll in `ScrollViewer`
2. Repeated tab switching in `TabView`
3. Burst toast creation and dismissal
4. Button hover and ripple spam
5. Text input with selection, IME, caret blinking, and focus transitions
6. Large `ListView` and `TreeView` interaction
7. Resize drag while animations are active
8. DPI change or simulated monitor switch
9. Transparent mode rendering
10. Maximize, restore, minimize, and reopen

Metrics to capture:

- average frame time
- p95 frame time
- number of dirty rects per frame
- cache hit rate
- number of rasterized nodes per frame
- layout passes per interaction

## Immediate Next Step

Begin with Phase 1 and create the core retained rendering infrastructure before attempting another partial-paint optimization pass.

