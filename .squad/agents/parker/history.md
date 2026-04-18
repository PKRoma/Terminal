# Project Context

- **Owner:** Mike Griese
- **Project:** OpenConsole — Windows Terminal, a modern terminal emulator for Windows
- **Stack:** C++/WinRT, XAML, WinUI 2.x, COM, Windows SDK
- **Created:** 2026-03-30

## Learnings

<!-- Append new learnings below. Each entry is something lasting about the project. -->

### 2026-03-30: Tab Overview plumbing
- **Action system pattern**: No-args actions go in `ALL_SHORTCUT_ACTIONS` only. Handler follows `_Handle{Action}` naming, calls the public method, sets `args.Handled(true)`.
- **Lazy XAML loading**: `x:Load="False"` elements are loaded via `FindName(L"ElementName")`. Returns null if not yet loaded. Same pattern used by CommandPalette and SuggestionsControl.
- **State booleans**: `_isInFocusMode`, `_isInOverviewMode`, etc. live in TerminalPage.h private section. Public getter is `FocusMode()`, `OverviewMode()`, etc. IDL property is `Boolean FocusMode { get; }`.
- **Tab content reparenting**: `_UpdatedSelectedTab(tab)` clears `_tabContent.Children()` and appends `tab.Content()`. Must be called after exiting overview to restore the active tab's content.
- **Key file locations**: AllShortcutActions.h (action registry), AppActionHandlers.cpp (handler impl), TerminalPage.h/.cpp (state + toggle logic), TerminalPage.xaml (UI element placement), TerminalPage.idl (WinRT interface), Resources.resw (localized strings), TerminalAppLib.vcxproj (build includes).
- **Dallas's OverviewPane IDL**: Uses `UpdateTabContent(IVector<Tab>, Int32)` and `ClearTabContent()`. Events: `TabSelected` (fires with selected index) and `Dismissed` (fires on Escape).

### 2026-03-30: MinimumSize assert fix
- **TermControl::MinimumSize() fallback**: When a pane splits, the new TermControl is added to XAML before its `Loaded` event fires. `_initializedTerminal` is false at that point, so `MinimumSize()` hits the else branch. The `{ 10, 10 }` fallback is correct — removed the `assert(false)` that was crashing debug builds on every split.
- **Key path**: `src/cascadia/TerminalControl/TermControl.cpp` ~line 2892.
- **Pattern**: Pane split flow: `Pane::_Split()` → `_SetupEntranceAnimation()` → `_CalcSnappedChildrenSizes()` → `_CreateMinSizeTree()` → `_GetMinSize()` → `MinimumSize()`. The new control hasn't loaded yet at that point.
