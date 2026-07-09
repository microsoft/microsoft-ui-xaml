#Requires -Version 5.0
<#
.SYNOPSIS
    Live DComp tree viewer -> writes a self-refreshing XML file.

.DESCRIPTION
    A checked (chk) build of Microsoft.UI.Xaml.dll emits the DComp tree as XML
    via OutputDebugString, wrapped in <DCompTreeSnapshot> ... </DCompTreeSnapshot>.
    This script captures each complete snapshot and OVERWRITES an .xml file, so
    the file always holds the latest tree. Open it in an editor that auto-reloads
    on change (e.g. VS Code) to watch it refresh live.

    Snapshots are emitted at most every ~2.5s and ONLY while the app is actively
    rendering. Interact with the app (scroll / expand / click) to drive updates.

    REQUIREMENTS:
      * The app must load THIS freshly-built chk Microsoft.UI.Xaml.dll.
      * No other capturer (DebugView / VS native debugger) may be attached -
        only one process can read OutputDebugString at a time.

.PARAMETER OutFile
    Where to write the XML. Default: live_dcomp_tree.xml next to this script.

.PARAMETER Seconds
    How long to capture before auto-stopping. Default 600 (10 min).

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\LiveDCompTree.ps1
#>
param(
    [string]$OutFile = "$PSScriptRoot\live_dcomp_tree.xml",
    [int]$Seconds = 600
)

# Seed the file immediately so it always exists, even before the first snapshot.
$seed = @"
<?xml version="1.0" encoding="utf-8"?>
<!-- Waiting for the app to render a snapshot...
     1. Run FolderTreeApp from the Start menu (so it loads the freshly-built chk DLL).
     2. Close DebugView / detach the VS native debugger (only one capturer allowed).
     3. Interact with the app (expand the tree) to keep it rendering.
     Started: $(Get-Date -Format 'HH:mm:ss') -->
<DCompTreeSnapshot />
"@
Set-Content -Path $OutFile -Value $seed -Encoding UTF8
Write-Host "XML file created (auto-refreshes on each snapshot): $OutFile"

$src = @"
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

public static class LiveDCompXml
{
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] static extern IntPtr CreateEvent(IntPtr a, bool m, bool s, string n);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] static extern IntPtr CreateFileMapping(IntPtr f, IntPtr sa, uint p, uint hi, uint lo, string n);
    [DllImport("kernel32.dll")] static extern IntPtr MapViewOfFile(IntPtr m, uint a, uint hi, uint lo, UIntPtr b);
    [DllImport("kernel32.dll")] static extern bool SetEvent(IntPtr h);
    [DllImport("kernel32.dll")] static extern uint WaitForSingleObject(IntPtr h, uint ms);

    const uint PAGE_READWRITE = 0x04;
    const uint FILE_MAP_READ  = 0x0004;
    static readonly IntPtr INVALID = new IntPtr(-1);

    // Returns: 0 = stopped normally, 1 = could not attach.
    public static int Run(string outFile, int seconds)
    {
        IntPtr bufferReady = CreateEvent(IntPtr.Zero, false, false, "DBWIN_BUFFER_READY");
        IntPtr dataReady    = CreateEvent(IntPtr.Zero, false, false, "DBWIN_DATA_READY");
        IntPtr hMap = CreateFileMapping(INVALID, IntPtr.Zero, PAGE_READWRITE, 0, 4096, "DBWIN_BUFFER");
        IntPtr pBuf = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, (UIntPtr)4096);

        if (bufferReady == IntPtr.Zero || dataReady == IntPtr.Zero || hMap == IntPtr.Zero || pBuf == IntPtr.Zero)
        {
            Console.WriteLine("ERROR: could not attach to OutputDebugString.");
            Console.WriteLine("Is DebugView or the VS native debugger already capturing? Close it and retry.");
            return 1;
        }

        Console.WriteLine("Live DComp XML viewer started.");
        Console.WriteLine("Press any key (or Ctrl+C) to stop. Auto-stops in " + seconds + "s.");
        Console.WriteLine("Waiting for a snapshot... (interact with the app)");
        SetEvent(bufferReady);

        var body = new List<string>();
        bool inSnapshot = false;
        int snapshotCount = 0;
        DateTime end = DateTime.Now.AddSeconds(seconds);

        bool canPollKeys = false;
        try { canPollKeys = !Console.IsInputRedirected; } catch { canPollKeys = false; }

        while (DateTime.Now < end)
        {
            if (canPollKeys)
            {
                try { if (Console.KeyAvailable) { Console.ReadKey(true); break; } }
                catch { canPollKeys = false; }
            }

            uint rc = WaitForSingleObject(dataReady, 300);
            if (rc != 0) continue;

            string msg = Marshal.PtrToStringAnsi(new IntPtr(pBuf.ToInt64() + 4));
            SetEvent(bufferReady);
            if (msg == null) continue;
            msg = msg.TrimEnd('\r', '\n');

            if (msg.Contains("<DCompTreeSnapshot>"))
            {
                inSnapshot = true;
                body.Clear();
            }
            else if (inSnapshot && msg.Contains("</DCompTreeSnapshot>"))
            {
                inSnapshot = false;
                snapshotCount++;

                // Build a well-formed XML document and write it atomically. The two trees
                // (<CompTree> + <VisualTree>) arrive as body lines between the markers, so
                // no format handling is needed here - they are emitted verbatim.
                var sb = new System.Text.StringBuilder();
                sb.AppendLine("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
                sb.AppendLine("<DCompTreeSnapshot capturedAt=\"" + DateTime.Now.ToString("HH:mm:ss") +
                              "\" snapshot=\"" + snapshotCount + "\">");
                foreach (var l in body) sb.AppendLine(l);
                sb.AppendLine("</DCompTreeSnapshot>");

                // Write to a temp file, then atomically replace the target. This guarantees a
                // live reader (e.g. the DCompTreeViewer.html "Watch file" auto-reload) never
                // sees a partially-written document and hits a spurious XML parse error.
                try
                {
                    var tmp = outFile + ".tmp";
                    File.WriteAllText(tmp, sb.ToString());
                    try { File.Replace(tmp, outFile, null); }
                    catch { File.Copy(tmp, outFile, true); try { File.Delete(tmp); } catch { } }
                }
                catch { }

                try { if (canPollKeys) Console.Clear(); } catch { }
                Console.WriteLine("File: " + outFile + "   |   press any key to stop");
                Console.WriteLine();
                Console.WriteLine(sb.ToString());
            }
            else if (inSnapshot)
            {
                body.Add(msg);
            }
        }

        Console.WriteLine("\r\nStopped. Captured " + snapshotCount + " snapshot(s).");
        return 0;
    }
}
"@

Add-Type -TypeDefinition $src -Language CSharp
Write-Host "Starting live XML capture..."
$rc = [LiveDCompXml]::Run($OutFile, $Seconds)
if ($rc -eq 1) { Write-Host "Could not attach - see message above." -ForegroundColor Red }
Write-Host "Done. Current tree is in: $OutFile"
