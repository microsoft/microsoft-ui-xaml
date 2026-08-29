# Window placement persistence (WindowPlacement)

This component implements the parts of WinUI window placement persistence that WinUI owns
outright: the versioned `WPL1` binary format that a saved placement is stored in, and the
deterministic derivation of the `LocalSettings` value name a placement is stored under.

See the design in `docs/design-notes/Window-PlacementPersistence.md`.

## What lives here

| Path | Contents |
| ---- | -------- |
| `inc/WindowPlacementData.h` | `WindowPlacementData` / `WindowPlacementRect`, the in-memory placement. |
| `inc/WindowPlacementFormat.h` | The `WPL1` wire-format contract: magic, version, tags, flag bits, and limits. |
| `inc/WindowPlacementSerializer.h` | Serialize / parse a `WindowPlacementData` to and from `WPL1`. |
| `inc/WindowPlacementValueName.h` | Derive the `wp1_<slug>_<hash>` value name from a `PersistPlacementId`. |
| `lib/` | The static library that implements the above. |
| `unittests/` | TAEF unit tests, including the byte-exact golden-blob test. |
| `inc/PlacementEx/` | Home for the PlacementEx sources ported from the OS repo (see below). |

## The WPL1 format

A saved placement is a small, versioned, tag-length-value blob. It is Base64-encoded before it
goes into `ApplicationData.LocalSettings`.

```
Header (12 bytes)
  char   magic[4]     'W' 'P' 'L' '1'
  uint16 major        must-understand; a reader rejects an unknown major
  uint16 minor        additive; a reader tolerates an unknown minor
  uint32 totalLength  whole blob length in bytes, including this header

Field (repeated until totalLength)
  uint16 tag
  uint16 length       length of value in bytes
  byte   value[length]
```

All integers are little-endian. Rectangle coordinates are signed `int32`; other scalars are
unsigned `uint32`. A device name is raw UTF-16LE with no trailing NUL. A virtual-desktop id uses
the Windows `GUID` field layout.

Rules that keep restore safe:

- The writer emits each present field once, in ascending tag order, so a given placement produces
  one canonical byte sequence. `GoldenBlobV1` pins that sequence.
- The parser is fail-safe. Any malformed, truncated, unknown-major, or oversized blob is reported
  as "no saved placement" rather than a partial result, so restore never applies half a state.
- A reader skips an unknown tag by its length and ignores unknown flag bits and unknown `SW_*`
  show commands, so a newer minor version stays readable by an older reader.
- `TAG_NORMAL_RECT` is required. A blob without it is not a usable placement.

## The value name

A `PersistPlacementId` is an arbitrary app string, so it is never used directly as a
case-insensitive `LocalSettings` value name. Instead:

```
valueName = "wp1_" + slug(rawId) + "_" + Base32(SHA-256(UTF-16LE(rawId)))
```

- `slug` keeps up to 16 ASCII alphanumerics from the raw id, or `id` if none qualify. It is
  diagnostic only.
- The uppercase, unpadded RFC 4648 Base32 of the SHA-256 digest (32 bytes -> 52 characters)
  supplies uniqueness and makes any app string safe as a value name.

## PlacementEx

PlacementEx is the shared placement math (monitor fit-up, restore-to-work-area, arranged/snapped
handling). It is owned by the OS repo and is ported into `inc/PlacementEx/`. See that folder's
docs for provenance and the concepts it implements. WinUI translates between `WindowPlacementData`
and PlacementEx at the native boundary; PlacementEx never sees the `WPL1` bytes.

## Tests

The tests are a standalone TAEF DLL and need no UI, so they run in-proc on a dev box:

```
te.exe BuildOutput\bin\amd64chk\Test\Microsoft.UI.Xaml.Tests.Isolated.WindowPlacement.dll /inproc
```
