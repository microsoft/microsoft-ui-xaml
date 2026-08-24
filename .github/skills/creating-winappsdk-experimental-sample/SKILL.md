---
name: creating-winappsdk-experimental-sample
description: Use when a new monthly experimental Windows App SDK (WinAppSDK) release ships and you need to scaffold, build, and launch a minimal self-contained WinUI 3 sample pinned to that exact package version to smoke-test the release. Also use when a WinUI 3 build fails with "Unable to find package Microsoft.Windows.SDK.NET.Ref" (only searches library-packs), or a NuGet credential-provider / private-feed error blocks restore of Microsoft.WindowsAppSDK experimental packages.
---

# Creating a WinAppSDK Experimental Sample

## Overview

Windows App SDK publishes an **experimental** channel roughly once a month
(e.g. `2.3.2-experimentalA`). This skill produces a tiny, known-good WinUI 3
desktop app pinned to a specific `Microsoft.WindowsAppSDK` version so the
release can be validated end-to-end: **restore → build → launch**.

The work is done by [`New-WinAppSdkSample.ps1`](New-WinAppSdkSample.ps1) in this
folder. It generates a minimal unpackaged, **self-contained** app (the WinAppSDK
runtime is bundled next to the exe, so no separate runtime install is needed).

## When to Use

- A new monthly WinAppSDK experimental package is out and you want a quick sanity app.
- You need a repro app against one exact experimental version.
- Symptoms this skill also fixes:
  - `error NU1101: Unable to find package Microsoft.Windows.SDK.NET.Ref ... source(s): ...\dotnet\library-packs`
  - `A plugin was not found at path '...CredentialProvider.Microsoft...'` during restore
  - Private-only NuGet feeds prompting auth when the package is on nuget.org

**Not for:** shipping/production apps, packaged MSIX store submissions, or adding a
control page to WinUI Gallery itself (edit `ControlInfoData.json` for that).

## Quick Reference

```powershell
$skill = ".github\skills\creating-winappsdk-experimental-sample\New-WinAppSdkSample.ps1"

# Auto-detect the latest experimental release, build, and launch:
& $skill -Launch

# Pin an exact version:
& $skill -Version 2.3.2-experimentalA -Launch

# Generate only (no build), into a chosen folder:
& $skill -Version 2.3.2-experimentalA -OutputDir C:\temp\wasdk-smoke -NoBuild

# ARM64:
& $skill -Version 2.3.2-experimentalA -Platform arm64 -Launch
```

| Parameter | Default | Purpose |
|---|---|---|
| `-Version` | latest `*experimental*` on nuget.org | `Microsoft.WindowsAppSDK` version to pin |
| `-OutputDir` | `$HOME\WinAppSdkSamples\WinUISample-<Version>` | where the app is generated |
| `-Platform` | `x64` | `x64` \| `x86` \| `arm64` |
| `-TargetFramework` | `net9.0-windows10.0.22621.0` | app TFM |
| `-WindowsSdkPackageVersion` | latest stable pack for the TFM band | `Microsoft.Windows.SDK.NET.Ref` version |
| `-Launch` | off | launch and verify the process stays alive |
| `-NoBuild` | off | generate files only |

## How It Works (and why)

1. **Resolves versions from nuget.org** — newest experimental WinAppSDK and a
   matching stable Windows SDK projection pack (`Microsoft.Windows.SDK.NET.Ref`).
2. **Generates the project** — `.csproj` (pins `Microsoft.WindowsAppSDK`),
   `App.xaml(.cs)`, `MainWindow.xaml(.cs)`, `app.manifest`, and an isolated
   `nuget.config` (`<clear/>` + nuget.org only).
3. **Neutralizes NuGet blockers** — sets `NUGET_PLUGIN_PATHS=""` so a broken/absent
   credential provider can't fail the restore.
4. **Handles the missing Windows SDK targeting pack** — the .NET SDK treats
   `Microsoft.Windows.SDK.NET.Ref` as a targeting pack and only searches the offline
   `library-packs` folder (it will **not** pull it from nuget.org, even as a plain
   `PackageReference`). When no pack is installed, the script downloads the `.nupkg`
   and builds a valid **v3 fallback package folder** (`<id>/<ver>/` + expanded
   contents + `.nupkg`, `.nupkg.sha512`, `.nupkg.metadata`), registered via
   `<fallbackPackageFolders>`. If a pack is already installed, this step is skipped.
5. **Builds and (optionally) launches**, verifying the exe exists and the process
   survives startup.

## Common Mistakes

- **Generating inside a repo that uses Central Package Management / `Directory.Build.props`.**
  The sample uses an explicit `PackageReference Version`, which CPM forbids. Keep
  `-OutputDir` **outside** such a repo (the default location already is).
- **Expecting a runtime install.** The app is self-contained/unpackaged — run the exe
  directly from `bin\<Platform>\Debug\<TFM>\`. No MSIX registration or runtime installer.
- **Pinning a `-WindowsSdkPackageVersion` that isn't on nuget.org.** Use one listed for
  the TFM's platform band (e.g. `10.0.22621.*`); the script auto-picks a valid one.
- **Ignoring `NU1603`.** A dependency patch-bump warning (e.g. build tools) is expected
  and harmless.

## Monthly Cadence

When the next experimental release drops, just re-run with the new version
(`& $skill -Version <new-version> -Launch`) or let auto-detect pick it. Each run is
self-contained in its own `-OutputDir`, so past samples are preserved for comparison.
