# Project Context

- **Owner:** Mike Griese
- **Project:** OpenConsole — Windows Terminal, a modern terminal emulator for Windows
- **Stack:** C++/WinRT, XAML, WinUI 2.x, COM, Windows SDK
- **Created:** 2026-03-30

## Learnings

<!-- Append new learnings below. Each entry is something lasting about the project. -->

- **2026-04-18 (from Dallas):** New `TerminalPage::_DismissOverviewVisuals()` helper is the canonical release-first teardown for OverviewPane reparented content. Call it from any external code path that may mount tab content (e.g. `_OnTabSelectionChanged`) before `_UpdatedSelectedTab` runs. Idempotent — safe to call when not in overview mode.
