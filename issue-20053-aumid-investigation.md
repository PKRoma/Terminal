# Issue #20053: Taskbar Shortcut Regression with Unpackaged AUMID

## Summary

PR [#20018](https://github.com/microsoft/terminal/pull/20018) introduced
`SetCurrentProcessExplicitAppUserModelID` for unpackaged builds so that future
features (toast notifications, shell integration) have a stable
AppUserModelID (AUMID). This caused a regression: when Windows Terminal is
pinned to the taskbar via a `.lnk` shortcut, launching the exe creates a
**separate** taskbar entry instead of grouping with the pin.

## Root Cause

The Windows shell resolves taskbar grouping identity in priority order:

1. Per-window AUMID (set via `IPropertyStore` on the HWND)
2. Per-process AUMID (`SetCurrentProcessExplicitAppUserModelID`)
3. Auto-derived from the executable path

Before #20018, neither the pinned `.lnk` nor the process had an explicit
AUMID. Both used the auto-derived identity (from exe path), so they matched
and the taskbar grouped them together.

After #20018, the process sets an explicit AUMID
(`WindowsTerminalDev.<exeHash>.<sidHash>`), but the pinned shortcut still has
no AUMID. The shell sees two different identities — the pin's auto-derived one
and the process's explicit one — and creates separate taskbar entries.

## Debugging Tools Used

### Inspecting shortcut AUMIDs

```powershell
$taskbarPath = "$env:APPDATA\Microsoft\Internet Explorer\Quick Launch\User Pinned\TaskBar"
$shell = New-Object -ComObject Shell.Application
$folder = $shell.NameSpace($taskbarPath)
foreach ($item in $folder.Items()) {
    $aumid = $item.ExtendedProperty("System.AppUserModel.ID")
    Write-Host "$($item.Name): AUMID = '$aumid'"
}
```

### Inspecting process AUMIDs

`SetCurrentProcessExplicitAppUserModelID` sets a process-level AUMID that
cannot be read from another process via any public API.
`SHGetPropertyStoreForWindow` only returns per-*window* AUMIDs (which are
separate). We used a memory-scanning PowerShell script
(`scratch/Get-ProcessAumid.ps1`) that scans the target process's committed
memory for UTF-16 strings matching the AUMID pattern. This confirmed which
builds were setting the AUMID.

### Key observation

- Shortcut after fresh pin: **no AUMID** (empty)
- Process before #20018: **no AUMID** (auto-derived from exe path) → matches pin
- Process after #20018: **explicit AUMID** (`WindowsTerminalDev.<hash>.<hash>`) → mismatch

## Attempted Fixes

### Attempt 1: Stamp the shortcut at launch, skip process AUMID

**Idea:** When launched from a shortcut (`STARTF_TITLEISLINKNAME`), check if
the `.lnk` has our AUMID. If not, stamp it and skip
`SetCurrentProcessExplicitAppUserModelID` for this launch.

**Result:** Only handled the "launched from shortcut" case. Double-clicking the
exe directly (not via the pin) still set the process AUMID, causing the same
mismatch.

### Attempt 2: Enumerate pinned taskbar shortcuts at launch

**Idea:** At startup, enumerate all `.lnk` files in the pinned taskbar folder
(`%APPDATA%\Microsoft\Internet Explorer\Quick Launch\User Pinned\TaskBar`).
Find any shortcut pointing to our exe. If it doesn't have our AUMID, stamp it
and skip the process AUMID.

**Result:** The stamping itself caused the shell to immediately re-read the
shortcut, updating the pin's cached identity mid-launch. The pin now had the
explicit AUMID while the running window (with no process AUMID set) still used
the auto-derived identity. Still a mismatch, just in the opposite direction.

### Attempt 3: Defer shortcut stamping to process exit

**Idea:** Same detection logic, but instead of stamping the shortcut during
startup, defer it to process shutdown (right before `TerminateProcess`). On
the first launch with an unstamped pin, skip both the stamp and the process
AUMID — the window groups correctly because both use the auto-derived identity.
At shutdown, stamp the shortcut. On the next launch, the shortcut has our
AUMID, so we set the process AUMID to match — both agree and grouping works.

**Result:** Works correctly.

## Final Fix

The fix is in `WindowEmperor::_setupAumid()` with three states:

| State | Pin exists? | Pin has our AUMID? | Action |
|---|---|---|---|
| `NotFound` | No | N/A | Set process AUMID (no conflict possible) |
| `AlreadyStamped` | Yes | Yes | Set process AUMID (they match) |
| `NeedsStamping` | Yes | No | Skip process AUMID; stamp shortcut at shutdown |

The shortcut stamping at shutdown uses the standard public COM APIs:
`IShellLinkW`, `IPersistFile`, `IPropertyStore`, and `PKEY_AppUserModel_ID`.

### Trade-off

On the **very first launch** after pinning (before the shortcut has been
stamped), the process AUMID is not set. This means any features that depend on
the process AUMID (e.g., toast notifications) won't work until the second
launch. This is acceptable because the first launch stamps the shortcut at
exit, so subsequent launches have the full AUMID active.

## Files Changed

- `src/cascadia/WindowsTerminal/WindowEmperor.cpp` — Added `_setupAumid()`
  method and shutdown stamping logic
- `src/cascadia/WindowsTerminal/WindowEmperor.h` — Added method declaration
  and `_pendingAumidLnkPath`/`_pendingAumid` member variables
