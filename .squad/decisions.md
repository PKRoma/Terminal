# Squad Decisions

## Active Decisions

### 2026-04-18: Overview must be dismissed on external tab selection (Dallas)

- **Decision:** External tab switches (TabView SelectionChanged driven by user clicks in the tab row) must tear down the OverviewPane's reparented content BEFORE `_UpdatedSelectedTab` runs.
- **Mechanism:** New helper `TerminalPage::_DismissOverviewVisuals()` performs the teardown (revoke `TabSelected`/`Dismissed` handlers, `ClearTabContent()`, hide overlay, clear `_isInOverviewMode`). It's idempotent — early-returns when not in overview mode.
- **Call sites:**
  - `_ExitOverview(...)` — replaces inline cleanup; the index-selection / `_UpdatedSelectedTab` step still happens after.
  - `_OnTabSelectionChanged(...)` — invoked at the top, inside the existing `!_rearranging && !_removing` guard.
- **Why not `_ExitOverview` from `_OnTabSelectionChanged`:** `_ExitOverview` calls `_SelectTab` which sets `_tabView.SelectedItem(...)`, which would re-fire `SelectionChanged` and risk reentry. The TabView already updated its selection, so we only need teardown.
- **General rule:** Any control that reparents XAML elements out of their normal tree must provide a release-first hook called by every external code path that may touch those elements.
- **Files:** `src/cascadia/TerminalApp/TerminalPage.h`, `src/cascadia/TerminalApp/TerminalPage.cpp`, `src/cascadia/TerminalApp/TabManagement.cpp`

### 2026-03-30: Tab Overview Feature — Architecture (Ripley)

- **Overlay + Reparent with ViewBox approach**: Overview is an overlay control in TerminalPage.xaml (same pattern as CommandPalette). Tab content is reparented into Viewbox cells in an overview grid, then restored on dismiss.
- **New files**: OverviewPane.xaml, .h, .cpp, .idl in `src/cascadia/TerminalApp/`
- **Action**: `toggleOverview` — no-args action, same pattern as `ToggleFocusMode`
- **State**: `_isInOverviewMode` bool on TerminalPage; `_EnterOverview()` / `_ExitOverview()` manage reparenting
- **Keyboard**: Arrow keys navigate grid, Enter selects, Escape dismisses
- **Animation**: Fade-in overlay (200ms), storyboard-based scale/opacity transitions

### 2026-03-30: ToggleOverview Action Wiring (Parker)

- **No-args action**: Added to `ALL_SHORTCUT_ACTIONS` only (not `WITH_ARGS`), follows `ToggleFocusMode` pattern
- **Lazy loading**: OverviewPane uses `x:Load="False"` + `FindName()` pattern (same as CommandPalette)
- **API adaptation**: Matched Dallas's IDL — `UpdateTabContent(IVector<Tab>, Int32)` and `ClearTabContent()` instead of originally spec'd names
- **OverviewMode property**: Added `Boolean OverviewMode { get; }` to IDL for XAML data binding

### 2026-03-30: OverviewPane UI Design (Dallas)

- **Layout**: ItemsControl + WrapGrid panel (lighter than GridView, manual selection management)
- **Preview sizing**: 320×220 Border containing Viewbox per cell, 12px gap between cells
- **Content reparenting**: `_DetachContent()` handles Panel/Border/ContentControl/Viewbox parents; `_reparentedContent` tracks originals; `ClearTabContent()` restores
- **Animation**: XAML Storyboard — enter fade 200ms, exit fade 150ms, QuadraticEase
- **Selection**: SystemAccentColor 2px border on selected; transparent 2px on others (no layout shift)
- **Overlay background**: #CC000000 (80% opacity black)

### 2026-03-30: Replace Viewbox with ScaleTransform in OverviewPane (Dallas)

- **Change**: OverviewPane tab previews now use `RenderTransform(ScaleTransform)` + explicit Width/Height + Canvas clip instead of Viewbox
- **Reason**: Viewbox causes layout-based scaling which triggers TermControl buffer reflows (column/row changes). ScaleTransform is render-only — applied after layout — so the control keeps its original logical size.
- **Impact**: `ReparentedEntry` struct now tracks originalWidth, originalHeight, and originalRenderTransform for clean restore.

### 2026-03-30: Remove assert(false) from TermControl::MinimumSize() (Parker)

- **Change**: Removed `assert(false)` in `TermControl::MinimumSize()` else branch. The assert was a diagnostic from PR #18027 (Nov 2024) that was hitting on every pane split.
- **Reason**: The `{ 10, 10 }` fallback was already correct behavior — new TermControl added to XAML tree before Loaded event fires during split. The assert crashed debug builds unnecessarily.
- **File**: `src/cascadia/TerminalControl/TermControl.cpp`

## Governance

- All meaningful changes require team consensus
- Document architectural decisions here
- Keep history focused on work, decisions focused on direction
