# OS Framework Lens

A small Windows developer tool that tells you, in real time, which **UI framework**
the element under your cursor belongs to — Win32, ComCtl32, **DirectUI**, WPF,
WinForms, **WinUI 2 (UWP XAML)**, **WinUI 3 (WindowsAppSDK)**, **React Native for
Windows**, **.NET MAUI**, MFC, Chromium / WebView2 / Electron, Qt, Java AWT/Swing,
or Office NetUI — along with the HWND class, window title, process, **loaded
framework runtime modules** (e.g. `dui70.dll`, `Microsoft.UI.Xaml.dll`), and the
raw UI Automation properties used to make the call.

Built because Windows UIs are heterogeneous — a single app window can mix
ComCtl, DirectUI, WPF, and WinUI islands — and the existing inspectors
(Spy++, Accessibility Insights, Inspect.exe) either don't surface the framework
explicitly or don't disambiguate WinUI 2 vs WinUI 3 vs RN-Windows vs MAUI.

## How it classifies

```
   ┌─ UI Automation `FrameworkId`   (per-element)        +5  ◄ primary
   ├─ HWND class-name regex         (per-window)         +4  ◄ strong
   └─ Process loaded modules        (per-process)        +1 each ◄ tie-breaker
```

The `+5 / +4 / +1` split is deliberate. Loaded modules are a *process-wide*
signal — `explorer.exe` simultaneously loads `dui70.dll` (DirectUI), `comctl32.dll`,
and `Microsoft.UI.Xaml.dll` (XAML islands). A naïve "this process has dui70.dll
loaded ⇒ element is DUI" would be wrong. Module evidence is only allowed to
**break ties** between candidates that both passed the per-element UIA / class
checks. That is also exactly how we tell **WinUI 2 vs WinUI 3** apart
(`Windows.UI.Xaml.dll` vs `Microsoft.UI.Xaml.dll`) and how we surface
**React Native for Windows** and **MAUI** (which both look like plain WinUI to
UIA, but ship distinguishing managed assemblies).

The full fingerprint table lives in [`src/OSFrameworkLens/Fingerprints/frameworks.json`](src/OSFrameworkLens/Fingerprints/frameworks.json)
and can be edited without recompiling the matcher logic.

## Classification reference

The exact scoring formula from [`FrameworkClassifier.cs`](src/OSFrameworkLens/Detection/FrameworkClassifier.cs):

```text
score = (UIA FrameworkId match ? 5 : 0)                            ← per-element
      + (any classRegex matches HWND class OR UIA ClassName ? 4 : 0) ← per-window
      + (count of matching modules)                                 ← per-process

confidence = score ≥ 5 → "High"
             score = 4 → "Medium"
             score = 1..3 → "Low"
             score = 0 → "None"  (returns the "Unknown" framework)
```

The framework with the highest total score wins. Ties break by JSON declaration order.

The class signal is checked against **both** the HWND class returned by `GetClassName` *and* the UIA element's `ClassName` property, because either can be more specific than the other:

- **XAML Islands** surface the bridge class through UIA (e.g. `Microsoft.UI.Content.DesktopChildSiteBridge`) while the HWND under the cursor is the host (`CabinetWClass`). The UIA name is what fingerprints the island.
- **DUI items** do the opposite — UIA reports a virtual ClassName like `UIItem`, but the HWND class `DirectUIHWND` is the framework-identifying string.

The +4 is credited once per framework even if both strings match the same regex.

### Per-framework decision table

Signal shorthand: **U** = UIA FrameworkId match (+5), **C** = class regex match against the HWND class *or* UIA ClassName (+4), **M**<sub>n</sub> = *n* matching modules (+n). Verbatim from [`Fingerprints/frameworks.json`](src/OSFrameworkLens/Fingerprints/frameworks.json).

| Framework (`id`) | UIA `FrameworkId` for U | Class regex(es) for C | Distinguishing module(s) for M | Typical combos → score → confidence | Concrete example |
|---|---|---|---|---|---|
| **WinUI 3 (WindowsAppSDK)** `winui3` | `XAML` | `WinUIDesktopWin32WindowClass`, `Microsoft.UI.Content.DesktopChildSiteBridge`, `Microsoft.UI.Content.*` | `Microsoft.UI.Xaml.dll`, `Microsoft.WindowsAppRuntime.dll` | U+C+M₁ = **10** → High · U+C = **9** → High · C = **4** → Medium · M₁ = **1** → Low | Win11 File Explorer command bar; Dev Home; PowerToys Settings |
| **WinUI 2 / UWP XAML** `winui2` | `XAML` | `Windows.UI.Core.CoreWindow`, `ApplicationFrameWindow`, `Windows.UI.Composition*` | `Windows.UI.Xaml.dll`, `Windows.UI.Xaml.Controls.dll` | U+C+M₁ = **10** → High · U+C = **9** → High · C = **4** → Medium | Windows Settings (pre-Win11); Calculator; Photos |
| **React Native for Windows** `react_native_windows` | `XAML` | *(none — `classRegex` is empty; RNW windows have the same HWND class as WinUI 3, so the WinUI 3 entry already owns it)* | `Microsoft.ReactNative.dll`, `Microsoft.ReactNative.Managed.dll`, `ReactCommon.dll`, `ReactNativeWindows.dll`, `hermes.dll`, `jsi.dll`, `v8jsi.dll`, `ChakraBridge.dll` | Per-element RNW only scores U = **5**. To beat WinUI 3's U+C+M₁ = **10** it needs U+M₅ = **10** (declared before `winui3` in JSON, so ties go to RNW). U+M₃ = **8** → High but loses to WinUI 3's 10. M₂ = **2** → Low | RNW gallery sample |
| **.NET MAUI (WinUI 3)** `maui` | `XAML` | *(none — same reason as RNW; MAUI windows are WinUI 3 windows)* | `Microsoft.Maui.dll`, `Microsoft.Maui.Controls.dll`, `Microsoft.Maui.Graphics.dll`, `Microsoft.Maui.Essentials.dll` | Per-element MAUI only scores U = **5**. Typical MAUI app loads all 4 Maui DLLs → U+M₄ = **9** which loses to WinUI 3's 10. Needs M₅ (e.g., MAUI DLLs + something else) to tie or win; in practice the 4 Maui DLLs + 1 WinUI 3 DLL count → MAUI still 9, WinUI 3 still 10 → **WinUI 3 wins by score**. Detection of MAUI today is unreliable for this reason | `dotnet new maui` Windows target |
| **Avalonia** `avalonia` | `Avalonia` (newer versions only) | *(none — per-instance class names like `Window_<random>`)* | `Avalonia.dll`, `Avalonia.Base.dll`, `Avalonia.Controls.dll`, `Avalonia.Win32.dll`, `Avalonia.Skia.dll`, `Avalonia.Desktop.dll` | U+M₃ = **8** → High · U = **5** → High · M₆ = **6** → High · M₁ = **1** → Low | Avalonia samples |
| **Flutter (Windows)** `flutter` | *(none — UIA emits nothing useful)* | `FLUTTER_RUNNER_WIN32_WINDOW`, `FlutterWindow` | `flutter_windows.dll` | C+M₁ = **5** → High · C = **4** → Medium | `flutter run -d windows` desktop binary |
| **DirectUI (DUI)** `dui` | `DirectUI` | `DirectUIHWND`, `CtrlNotifySink`, `DUIViewWndClassName`, `DetailsPaneHwndHostClass`, `SHELLDLL_DefView` | `dui70.dll`, `duser.dll` | U+C+M₂ = **11** → High · U+C = **9** → High · C = **4** → Medium | File Explorer items pane; classic Start menu jump list |
| **WPF** `wpf` | `WPF` | `HwndWrapper[…]` | `PresentationCore.dll`, `PresentationFramework.dll`, `wpfgfx_v0400.dll`, `wpfgfx_cor3.dll` | U+C+M₃ = **12** → High · U+C = **9** → High · C = **4** → Medium · M₃ = **3** → Low | Visual Studio designer; SnoopWPF; many LOB apps |
| **WinForms** `winforms` | `WinForm` | `WindowsForms10.*` | `System.Windows.Forms.dll` | U+C+M₁ = **10** → High · U+C = **9** → High · C = **4** → Medium · M₁ = **1** → Low | Legacy LOB dialogs; older Office add-in surfaces |
| **MFC** `mfc` | `Win32` *(shared with comctl32 / win32 / java / office)* | `Afx:*` | `mfc140.dll`, `mfc140u.dll`, `mfc120.dll`, `mfc120u.dll`, `mfc110.dll`, `mfc100.dll`, `mfc42.dll` | U+C+M₁ = **10** → High · U+C = **9** → High · C = **4** → Medium | Older MMC snap-ins; VS legacy dialogs |
| **Common Controls (ComCtl32)** `comctl32` *(family=`win32`)* | `Win32` | `SysListView32`, `SysTreeView32`, `SysTabControl32`, `SysHeader32`, `SysLink`, `msctls_*`, `ToolbarWindow32`, `ReBarWindow32`, `tooltips_class32` | `comctl32.dll` | U+C+M₁ = **10** → High · C+M₁ = **5** → High · C = **4** → Medium | A `SysTreeView32` leaf inside almost any classic Win32 app |
| **Chromium / WebView2 / Electron** `chromium` | `Chrome` | `Chrome_WidgetWin_*`, `Intermediate D3D Window`, `Chrome_RenderWidgetHostHWND` | `msedgewebview2.exe`, `WebView2Loader.dll`, `libcef.dll`, `chrome_elf.dll` | U+C+M₂ = **11** → High · U+C = **9** → High · C = **4** → Medium | Edge tabs; Slack; VS Code window chrome; WebView2 hosts |
| **Qt** `qt` | `Qt` | `Qt5*`, `Qt6*` | `Qt5Core.dll`, `Qt5Gui.dll`, `Qt5Widgets.dll`, `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll` | U+C+M₃ = **12** → High · U+C = **9** → High · C = **4** → Medium | OBS Studio; KDE-on-Windows ports |
| **Java AWT / Swing** `java` | `Win32` | `SunAwt*` | `awt.dll`, `jvm.dll`, `javaw.exe` | U+C+M₂ = **11** → High · U+C = **9** → High · C = **4** → Medium | IntelliJ IDEA; Eclipse; older JetBrains tools |
| **Office NetUI** `office_netui` | `Win32` | `NetUI*`, `NUIDialog`, `NUIHWND` | `mso.dll`, `mso20win32client.dll`, `mso30win32client.dll`, `msointl.dll` | U+C+M₃ = **12** → High · U+C = **9** → High · C = **4** → Medium | Word / Excel / Outlook ribbons and dialogs |
| **Win32 / USER32 (classic)** `win32` | `Win32` | `Button`, `Edit`, `Static`, `ListBox`, `ComboBox`, `ComboLBox`, `ScrollBar`, `#32770` (dialog), `#32768/9/71` (menu/desktop/icon title), `CabinetWClass`, `ExploreWClass`, `Shell_TrayWnd`, `Shell_SecondaryTrayWnd`, `Notepad`, `ConsoleWindowClass`, `Progman`, `WorkerW`, `TaskListThumbnailWnd`, `MsoCommandBar*`, `NamespaceTreeControl`, `ShellTabWindowClass`, `TITLE_BAR_SCAFFOLDING_WINDOW_CLASS`, `TopLevelWindowForOverflowXamlIsland` | `user32.dll`, `gdi32.dll` *(near-universal — almost every process loads them)* | U+C+M₂ = **11** → High · U+C = **9** → High · C = **4** → Medium · M₂ = **2** → Low | Classic Notepad; the `CabinetWClass` root of File Explorer |

### Where modules are the *only* tie-breaker

For these collisions, UIA + class score the same across multiple frameworks; the +1-per-module signal is what disambiguates:

| Collision | Per-element scores (U + C) | Module signal that decides |
|---|---|---|
| WinUI 2 vs WinUI 3 | both score `U(+5)` from `XAML`. Class regexes are **disjoint**, so the moment either side's class regex matches, that side scores `U+C = 9` and wins outright (modules irrelevant). | Modules only decide in the narrow case where **neither** class regex matches — e.g. a deep child XAML element whose UIA `ClassName` is a control-type string like `Button` and whose HWND class isn't a registered XAML window class. Both then sit at `U = 5`; `Windows.UI.Xaml.dll` → WinUI 2, `Microsoft.UI.Xaml.dll` → WinUI 3 |
| WinUI 3 vs RNW vs MAUI | only **WinUI 3** owns the `WinUIDesktopWin32WindowClass` regex → it scores **U+C = 9** while RNW and MAUI score only **U = 5**. WinUI 3 also picks up `+1` from `Microsoft.UI.Xaml.dll` → typical baseline **10** | RNW/MAUI must out-score 10 via modules. RNW needs **5+ `Microsoft.ReactNative.*`** DLLs (U+M₅ = 10, wins ties by JSON order — RNW is declared first). MAUI typically loads only 4 `Microsoft.Maui.*` DLLs (U+M₄ = 9) → **WinUI 3 still wins**; MAUI detection is a known gap |
| MFC / ComCtl32 / Win32 / Java / Office NetUI | all map `UIA=Win32` → `+5`; class regex usually disambiguates | When class doesn't hit: `mfc*.dll` → MFC; `awt.dll`/`jvm.dll` → Java; `mso*.dll` → Office; only `comctl32.dll` → ComCtl |
| Older Avalonia vs anything | no UIA, no class — scores 0 from per-element evidence | Only `Avalonia.*` presence can produce a non-zero score |

#### Why RNW and MAUI windows inherit the WinUI 3 class

`react_native_windows` and `maui` have **empty `classRegex`** in `frameworks.json` because both are *layered on top of* WinUI 3 — they aren't peers. RNW renders through WinUI 3's XAML/composition stack with a JavaScript bridge (Hermes / V8 / Chakra) on top; MAUI's Windows target *is* WinUI 3 with a different programming model. So a RNW or MAUI top-level window literally **is** a WinUI 3 window: same HWND class (`WinUIDesktopWin32WindowClass`), same UIA `FrameworkId=XAML`, same XAML tree. The only thing that distinguishes them from a plain WinUI 3 app is the **higher-level runtime DLLs** (`Microsoft.ReactNative.*`, `Microsoft.Maui.*`) loaded alongside the WinUI 3 stack — which is exactly what the per-process module signal captures. The architectural truth "an RNW window is a WinUI 3 window driven by an RNW app framework" is faithfully reflected by the scoring: WinUI 3 wins on per-element evidence, and a sufficient mass of RNW/MAUI modules in the process bumps the label up to the more useful application-framework name.

### Why scoring, not "DLL ⇒ framework" rules

A natural simplification is: "if `dui70.dll` is loaded, this HWND is DUI; if `Microsoft.UI.Xaml.dll` is loaded, it's WinUI 3; if `Windows.UI.Xaml.dll` is loaded, it's WinUI 2." That rule-based design is **wrong for this tool's core use case** because **a single process routinely loads multiple smoking-gun DLLs simultaneously**:

| Process | Smoking-gun DLLs co-loaded | Surfaces in that one process |
|---|---|---|
| `explorer.exe` (Win11) | `dui70.dll` **and** `Microsoft.UI.Xaml.dll` **and** `comctl32.dll` | DUI items pane · WinUI 3 command bar (XAML island) · `CabinetWClass` Win32 root · `SysTreeView32` nav leaf |
| `winword.exe` | `mso.dll` **and** `mfc*.dll` **and** WebView2 DLLs | NetUI ribbon · MFC legacy dialogs · Help pane (Chromium) |
| `devenv.exe` | WPF DLLs **and** `System.Windows.Forms.dll` **and** WebView2 | WPF main shell · WinForms legacy designers · WebView2 markdown preview |

Applying "`dui70.dll` loaded ⇒ HWND is DUI" inside Explorer would mislabel the Win11 command bar as DUI — exactly the multi-UI-thread bug the tool exists to *avoid*. Per-process module evidence is **necessary but not sufficient**: it tells you a framework is in use *somewhere* in the process, never that *this* HWND uses it.

The scoring formula IS the decision tree you'd otherwise write by hand, just expressed uniformly. The `+5 / +4 / +1` weights bake in the priority **per-element beats per-window beats per-process** so that:

- On clean HWNDs, per-element evidence (U + C ≥ 9) wins outright and the module bonus is just free corroboration — same answer a rule-based design would give.
- On islands and shared-class collisions (WinUI 3 vs RNW vs MAUI, MFC vs ComCtl32 vs Office NetUI), modules tie-break only after per-element evidence has had its say.
- When per-element evidence disagrees with modules (UIA says `Win32`, class is generic `Edit`, but `dui70.dll` is loaded in the process), the design correctly trusts the per-element verdict — the generic `Edit` is probably a plain Win32 textbox spawned next to DUI content, not a DUI surface itself.

The tool reports **one** framework per HWND — the per-element verdict. We deliberately don't surface a separate "process dominant framework" answer, because two labels on one element would be more confusing than useful; the right place for "what's loaded in this process" is the signal breakdown, not the headline.

### Why per-process modules can't distinguish per-HWND surfaces in the same process

A single process exposes the same module list to every HWND it owns. So inside one `explorer.exe`, the jump list, items pane, command-bar XAML island, and `CabinetWClass` root *all* see the same DLLs loaded. Modules add the same +1 baseline to every candidate and never flip the per-window winner:

| Explorer.exe surface | UIA | Class | DUI score | WinUI 3 score | Win32 score | Decision |
|---|---|---|---|---|---|---|
| Jump list popup | `DirectUI` | `DirectUIHWND` | **10** | 1 | 1 | DirectUI (High) |
| Items pane | `DirectUI` | `DirectUIHWND` | **10** | 1 | 1 | DirectUI (High) |
| Win11 command bar (XAML island) | `XAML` | `Microsoft.UI.Content.DesktopChildSiteBridge` | 1 | **10** | 1 | WinUI 3 (High), tagged **Island** because root HWND class is `CabinetWClass` (different family) |
| Address bar root | `Win32` | `CabinetWClass` | 1 | 1 | **10** | Win32 (High) |

That's why the +5 / +4 / +1 weighting is deliberate: per-window evidence does all the disambiguation, and modules only break *between-framework* ties that per-window evidence couldn't.

### How module evidence is collected

From [`ModuleInspector.cs`](src/OSFrameworkLens/Detection/ModuleInspector.cs):

1. **2-second per-PID cache** — the mouse moves many times per second; module lists barely change. Re-enumerating each frame would be wasteful. The cache also keeps a pre-lowered hashset of module names so per-tick classification doesn't re-allocate one each frame.
2. **`OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, pid)`** — minimum rights for module enumeration. If this fails (protected process, insufficient access), `Available=false` is returned and the caller must NOT treat the empty list as "no framework loaded".
3. **Two-call `EnumProcessModulesEx(hProc, …, LIST_MODULES_ALL)`** → `GetModuleFileNameExW` per handle → `Path.GetFileName` to keep only the file name. Matching is case-insensitive against the file name (never the full path).
4. **32-bit (WOW64) targets** — a 64-bit caller can't reliably enumerate a 32-bit target's module list (`ERROR_PARTIAL_COPY`). Those processes report `Available=false`; the classifier degrades gracefully to UIA + class-only evidence.

### Confidence bands at a glance

| Score | Label | Meaning |
|---|---|---|
| **0** | *None* → shown as `Unknown` (`#404040`) | No fingerprint matched |
| **1–3** | **Low** | Only module evidence — tells you a runtime is *loaded* in the process, not that this HWND uses it |
| **4** | **Medium** | Class regex matched but UIA disagreed (common for MFC's `Afx:` windows that UIA labels generically as `Win32`) — per-window evidence is still solid |
| **5–8** | **High** | One strong signal (UIA *or* class) plus at least one corroborating signal |
| **9+** | **High** | UIA + class agree (often with modules too) — strongest classification |

#### Same total, different meaning

The confidence label only looks at the *total*, but the *composition* matters for interpretation:

| Total | Could be… | Means |
|---|---|---|
| **4** | `C` (class match alone) | Strong per-window evidence — this HWND is that framework's surface |
| **4** | `M×4` (four matching modules, no U, no C) | Per-process evidence only — the runtime is loaded *somewhere* in the process but this specific HWND probably isn't using it |
| **5** | `U` alone | Per-element UIA verdict — strong but unconfirmed by HWND class or DLLs |
| **5** | `C + M₁` | Per-window class match corroborated by one module |
| **5** | `M×5` | Pure module count — same caveat as `M×4` |
| **8** | `U + M₃` | Element + runtime agree, but no class corroboration (typical Avalonia / RNW pattern) |
| **8** | `C + M₄` | Window class + runtime agree, but UIA emitted something else (typical Java / older Office) |
| **9** | `U + C` | Per-window verdict from *two independent* signals — the gold standard for this tool |

The classifier doesn't expose composition in the headline score, but the per-framework signal breakdown is what to look at when two frameworks tie or when a result feels surprising.

### How the per-element weighting solves the multi-UI-thread problem

A single process can host several UI frameworks on different threads (Office, `explorer.exe`, Visual Studio, …). Per-process signals like loaded modules **cannot** distinguish them — every HWND in the process sees the same DLL list.

The weights `+5` (UIA, per-element) and `+4` (class regex, per-window) vs `+1` (module, per-process) are deliberately tuned so that **per-window evidence dominates per-process noise**. Concretely:

- The maximum a single framework can earn from modules in a typical process is `M₃`–`M₄` (= 3–4 points). That can never outscore another framework's `U + C` (= 9) on the same HWND.
- The Explorer.exe table above shows the consequence: four surfaces in one process get three different framework decisions, even though all four see the *same* loaded DLLs.

This is why the existing **[Multi-UI-thread processes](#multi-ui-thread-processes-why-per-element-matters)** section just below works — the scoring formula itself encodes the per-element bias, and the table here is the receipt that it does.

## Multi-UI-thread processes (why per-element matters)

Some processes — most famously `explorer.exe`, but also Office apps and many shell
hosts — run multiple UI threads, each potentially driving a different framework:

- File Explorer's items view (**DirectUI**) and the Win11 command bar (**WinUI 3 island**)
  live in the same `explorer.exe` process on different UI threads.
- The Start menu, jump list, and search surfaces are spread across
  `StartMenuExperienceHost.exe` and `ShellExperienceHost.exe`, but each of *those*
  also hosts multiple frameworks.

Implications baked into the tool:

1. **Per-HWND attribution is mandatory.** We never infer the framework from
   "process has X.dll loaded" alone.
2. **Cross-thread probes use `SendMessageTimeout` with a 50 ms abort-if-hung budget.**
   `GetWindowText` via plain `SendMessage` can deadlock the inspector if a target
   UI thread is blocked.
3. **UIA `ElementFromPoint` runs in addition to `WindowFromPoint`/`ChildWindowFromPointEx`.**
   XAML Islands (`DesktopWindowXamlSource`) live on a separate dispatcher inside
   the host process; only UIA can see into them.

## Running

Requires Windows 10/11.

### Option A — double-click (no .NET install needed)

```powershell
.\publish.ps1
```

Produces a self-contained, single-file bundle at `dist\OSFrameworkLens\`:

| File | Size | Purpose |
|---|---|---|
| `OSFrameworkLens.exe` | ~74 MB | main app (win-x64, all .NET runtime + Windows App SDK embedded) |

Copy the `dist\OSFrameworkLens` folder anywhere and double-click `OSFrameworkLens.exe`. No prerequisites.

### Option B — from source

Requires the .NET 9 SDK.

```powershell
dotnet build
dotnet run --project src\OSFrameworkLens
```

A small dark inspector docks at the top-right; a colored rectangle follows the
hovered element. Press **Ctrl+Shift+F** to freeze the snapshot so you can
interact with the inspector. Click **Copy report** to put a Markdown summary on
the clipboard.

## Reuse of Accessibility Insights / Spy++

- **Accessibility Insights for Windows** (MIT) and its `Axe.Windows` library are
  thin UIA wrappers. We talk directly to `IUIAutomation` via a small COM interop
  in `Detection/UiaInspector.cs` for the same purpose, avoiding any extra
  dependency. Forking A11y Insights itself was rejected — it's a large audit
  tool, not an inspection scaffold.
- **Spy++** is closed-source and non-redistributable. Useful only as reference
  for HWND class-name heuristics.

## v1 scope / known gaps

- **64-bit only.** The main inspector is 64-bit and does not ship an x86
  sidecar. A 64-bit caller cannot reliably `EnumProcessModulesEx` a WOW64
  (32-bit) target, so those processes get classified by UIA + class-name
  evidence only — the module signal is reported as "unavailable" with that
  reason called out in the UI. Adding a 32-bit helper exe was deliberately
  removed to keep the deploy story to a single binary.
- **Per-monitor DPI:** the highlight overlay is sized in primary-monitor DIPs.
  Works correctly on single-DPI setups; mixed-DPI multi-monitor will be addressed
  by switching to per-monitor host overlays.
- **Elevated targets** (Task Manager, UAC dialogs) require the tool itself to run
  elevated. The manifest requests `asInvoker` so the choice is the user's.

## XAML Island detection

When the leaf HWND under the cursor classifies as one framework (e.g. WinUI 3)
but the *root* (top-level) HWND of the same window classifies as a different
framework (e.g. classic Win32 `CabinetWClass`), the inspector tags the result
as an **Island** and shows the host framework + root class name. This is the
Win11 File Explorer command bar case (WinUI 3 island inside a Win32 host) and
the Outlook compose surface case.

## Layout

```
src/OSFrameworkLens/
  Fingerprints/frameworks.json     – classifier rules (editable, embedded)
  Detection/Native.cs              – P/Invoke surface
  Detection/HwndInspector.cs       – cursor → HWND chain, safe class/title fetch
  Detection/ModuleInspector.cs     – per-PID module list (2s cache)
  Detection/UiaInspector.cs        – IUIAutomation COM interop (ElementFromPoint)
  Detection/FrameworkClassifier.cs – scoring + island host classification
  Detection/SnapshotService.cs     – orchestrates one snapshot
  UI/InspectorWindow.xaml(.cs)     – dark panel, polling timer, hotkey
  UI/HighlightOverlay.cs           – transparent click-through rectangle (raw Win32)
  UI/InspectorViewModel.cs         – binding model + clipboard report
```

## License

MIT. See [LICENSE](LICENSE).
