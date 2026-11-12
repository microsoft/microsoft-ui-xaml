# PlacementEx

PlacementEx captures a top-level window's normal bounds, state, and display
information, then adjusts that placement for the available displays.
See [RememberingWindowPositions.md](RememberingWindowPositions.md) for its
recommendations and [Win32Concepts.md](Win32Concepts.md) for native window behavior.

These headers use the repository's MIT license. We may want to keep them in
sync with other copies in the future.

## WinUI integration

Define `WINUI_PLACEMENTEX` before including `User32Utils.h`. WinUI adaptations
remain conditional so they can be compared with the original code:

- Exclude registry, string serialization, and monitor-topology helpers.
- Use thread-safe API lookup and explicit Windows callback conventions.
- Compile `ApplyWindowAction` in the existing newer-SDK light-up translation
  unit with `WINUI_PLACEMENTEX_LIGHTUP`; keep ordinary callers on their
  precompiled header (PCH).