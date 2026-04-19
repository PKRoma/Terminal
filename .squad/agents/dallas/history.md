# Project Context

- **Owner:** Mike Griese
- **Project:** OpenConsole — Windows Terminal, a modern terminal emulator for Windows
- **Stack:** C++/WinRT, XAML, WinUI 2.x, COM, Windows SDK
- **Created:** 2026-03-30

## Learnings

<!-- Append new learnings below. Each entry is something lasting about the project. -->

### 2026-03-30: OverviewPane control created
- Created OverviewPane.idl, .xaml, .h, .cpp — a full-screen grid overlay of tab previews
- Project uses `til::typed_event` for events, `BASIC_FACTORY` macro for factory impl
- Overlay controls in TerminalPage live in Grid.Row="2" with `Visibility="Collapsed"` and `x:Load="False"`
- Animation pattern: use Storyboard resources in XAML with DoubleAnimation + QuadraticEase (see Pane.cpp for C++ animation creation)
- Tab content reparenting: must call `_DetachContent()` before moving to a new parent (XAML single-parent rule). VisualTreeHelper::GetParent to find current parent.
- Tab.idl exposes `Content { get; }` as `FrameworkElement` — this is what we put in a Viewbox for the preview
- CommandPalette and SuggestionsControl use `RegisterPropertyChangedCallback(UIElement::VisibilityProperty())` to init when made visible
- Header pattern: `#include "pch.h"` first, then own header, then `.g.cpp`, then usings
- ItemsControl with WrapGrid panel template gives auto-wrapping grid layout

### 2026-03-30: OverviewPane ScaleTransform migration
- Replaced Viewbox-based content reparenting with ScaleTransform + explicit-size approach
- Key insight: `RenderTransform` is applied AFTER layout — content keeps its original logical size during Measure/Arrange
- Pattern: capture ActualWidth/ActualHeight before detaching, set explicit Width/Height, apply ScaleTransform, use Canvas+RectangleGeometry clip
- `ReparentedEntry` struct stores originalWidth, originalHeight, originalRenderTransform for clean restore in ClearTabContent
- Canvas is ideal intermediate container: measures children with infinite space, arranges at desired size
- Setting Width/Height back to NaN restores auto-sizing behavior
- Files: `src/cascadia/TerminalApp/OverviewPane.h`, `src/cascadia/TerminalApp/OverviewPane.cpp`

### 2026-04-18: OverviewPane reparenting crash on external tab switch
- **Bug:** Clicking a tab in the tab row while OverviewPane was open threw `0x800F1000 'Element is already the child of another element.'` `TabView.SelectionChanged` -> `_OnTabSelectionChanged` -> `_UpdatedSelectedTab` tried to mount tab Content while the overview still held it reparented in a preview cell.
- **Fix:** Extracted overview teardown (revoke handlers, `ClearTabContent()`, hide, clear `_isInOverviewMode`) into `TerminalPage::_DismissOverviewVisuals()`. `_ExitOverview` now calls it for the cleanup half. `_OnTabSelectionChanged` calls it at the top (after the rearranging/removing guard) so reparented content is restored to its original parent BEFORE `_UpdatedSelectedTab` runs.
- **Rule:** External tab switches (anything driven by `TabView.SelectedIndex` outside our own `_SelectTab` path) must tear down the overview visuals BEFORE `_UpdatedSelectedTab` runs. Do NOT call `_ExitOverview` from `_OnTabSelectionChanged` — it would re-enter `_SelectTab` -> `_tabView.SelectedItem(...)` -> `SelectionChanged` again.
- **Pattern:** Whenever a control reparents XAML elements out of their normal tree, every external code path that touches those elements needs a "release first" hook. A small idempotent `_Dismiss*Visuals()` helper (early-return when not active) is the right shape.
- **Files:** `src/cascadia/TerminalApp/TerminalPage.h`, `src/cascadia/TerminalApp/TerminalPage.cpp`, `src/cascadia/TerminalApp/TabManagement.cpp`

### 2026-04-19: Overview keybinding passthrough
- Overlay elements that reparent tab content (OverviewPane, CommandPalette, SuggestionsControl) hook `PreviewKeyDown="_KeyDownHandler"` on the overlay element in `TerminalPage.xaml`. This routes unrecognized keychords to TerminalPage's global action dispatch while the overlay is visible. Local keys stay local by being marked Handled in the control's `_OnKeyDown`.

### 2026-04-19: Overview pre-dismiss before DoAction
- In `_KeyDownHandler`, after the keychord resolves to a command and BEFORE `_actionDispatch->DoAction(...)`, call `_DismissOverviewVisuals()` when `_isInOverviewMode && action != ShortcutAction::ToggleOverview`. Overview reparents live tab `Content()`, so global actions that mutate tab state (NewTab/CloseTab/SplitPane/etc.) must see a restored XAML tree. Unlike `CommandPalette`, which can be dismissed after DoAction, overview MUST dismiss first. `ToggleOverview` is excluded — its animated exit path owns teardown.
