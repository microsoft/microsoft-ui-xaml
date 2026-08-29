# winui-xamlsplore.js

A WinDbg JavaScript debugger extension that generates an interactive HTML visual tree explorer
for WinUI3 (Microsoft.UI.Xaml) applications.

## What it does

When loaded in WinDbg, `winui-xamlsplore` walks the XAML visual tree of a WinUI3 app and
generates an HTML file (`c:\dumps\splore.html`) that lets you interactively explore the tree,
view element properties, filter by name/property, and visualize element bounds on a canvas.

## Usage

1. Break into a WinUI3 app in WinDbg.
2. Optionally load `winui-dbgext.js` first for richer element info:
   ```
   .scriptrun c:\path\to\winui-dbgext.js
   ```
3. Load this extension:
   ```
   .scriptrun c:\path\to\winui-xamlsplore.js
   ```
4. Run the command:
   ```
   !xamlsplore
   ```
5. Open `c:\dumps\splore.html` in a browser to explore the visual tree.

### `!xamlsploredumper`

An alternative command that dumps the visual tree directly to the debugger console
instead of generating an HTML file. By default it prints a text tree followed by JSON.
Pass `true` to output only the JSON representation:
```
!xamlsploredumper true
```

## How it works

The extension finds the XAML visual tree root by:
1. Reading `DXamlInstanceStorage::g_dwTlsIndex` from TLS to locate `DXamlCore`
2. Navigating `DXamlCore.m_hCore.m_pMainVisualTree.m_rootVisual` to get the root visual
3. Recursively walking `CUIElement.m_pChildren` (a `CompactInlineVector` accessed via `m_data`/`m_size`)
4. Collecting element types, bounds, names, and sparse properties into a JSON tree
5. Embedding the JSON into a self-contained HTML file with an interactive tree viewer
