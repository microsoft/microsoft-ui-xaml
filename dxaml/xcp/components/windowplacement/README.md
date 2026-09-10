# Window placement native helpers

This component contains the PlacementEx header-only window placement implementation and design
guidance used as the native basis for WinUI window placement persistence.

## Source

The files were copied from the Microsoft OS repository at commit
`f0d35ebfccaae9bdec48e2eb29e08f6224579209` on `official/ge_current`.

The copied files are maintained by WinUI under this repository's MIT license.

Local changes from the upstream files are limited to:

- adding the WinUI MIT header;
- updating PlacementEx's README reference; and
- correcting relative documentation links; and
- removing trailing whitespace.

## Design guidance

- [Remembering Window Positions](inc/PlacementEx/RememberingWindowPositions.md)
- [Win32 Windowing Concepts](inc/PlacementEx/Win32Concepts.md)

## Layout and use

Headers are isolated under `inc/PlacementEx` because several have generic names. Add the component's
`inc` directory to the consuming project's include path and include:

```cpp
#include <PlacementEx/User32Utils.h>
```

`User32Utils.h` is the entry point. It includes the platform and standard-library dependencies before
including PlacementEx and its supporting headers.

The component is checked in for design review and future implementation work. It is not yet wired
into a WinUI build target.

## Updating

When updating from the OS repository:

1. Record the new source commit above.
2. Copy the same source files.
3. Preserve the WinUI MIT header.
4. Reapply the documented local changes above.
5. Review upstream changes rather than treating the update as generated output.
6. Run both the native and forced-downlevel PlacementEx test paths after build integration.
