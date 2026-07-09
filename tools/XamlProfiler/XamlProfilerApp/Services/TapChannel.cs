using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Threading;
using Microsoft.Win32.SafeHandles;

namespace XamlProfiler.Services;

/// <summary>
/// Slim port of the WinUISnoop injector (TapHandler.cs). Injects the prebuilt
/// WinUISnoopTap.dll into a target WinUI process via XAML Diagnostics and drives
/// it with a tiny command protocol so the profiler can highlight a live element
/// on screen (Ctrl+Click cross-app highlight).
///
/// We only use a one-way command channel here (profiler -> tap): HIGHLIGHT / CLEAR.
/// The tap auto-enumerates the live visual tree on connect (AdviseVisualTreeChange),
/// which populates its RuntimeObjectCache so the producer-emitted peer handles
/// (IInspectable identity) resolve to live elements.
/// </summary>
internal sealed class TapChannel : IDisposable
{
    private readonly Process _target;
    private readonly object _writeLock = new();

    // Tap -> profiler pipe (we read "Connected"/tree messages, mostly ignored).
    private SafeFileHandle? _readHandle;
    private SafeFileHandle? _writeHandle;

    // Profiler -> tap pipe (we write HIGHLIGHT/CLEAR/CLOSE).
    private SafeFileHandle? _readHandleToTap;
    private SafeFileHandle? _writeHandleToTap;
    private FileStream? _fileStreamToTap;
    private StreamWriter? _streamWriterToTap;

    private Thread? _worker;
    private IntPtr _tapModuleHandle = IntPtr.Zero;
    private volatile bool _connected;
    private volatile bool _stopped;
    private ulong _pendingHighlight;

    public TapChannel(Process target)
    {
        _target = target ?? throw new ArgumentNullException(nameof(target));
    }

    public bool IsConnected => _connected;

    /// <summary>Raised on the worker thread when the tap reports "Connected".</summary>
    public event Action? Connected;

    /// <summary>
    /// Raised on the worker thread when the user picks an element in the target app
    /// (pick mode). The argument is the picked element's XAML-diagnostics handle,
    /// which equals the matching tree node's <c>PeerHandle</c>.
    /// </summary>
    public event Action<ulong>? ElementPicked;

    /// <summary>
    /// Raised on the worker thread alongside <see cref="ElementPicked"/>: the composition
    /// visual id (an IVisual*, from the clicked element's GetElementVisual — its xpid stamp
    /// or raw pointer). The profiler glows that visual node's whole subtree.
    /// </summary>
    public event Action<ulong>? VisualPicked;

    /// <summary>Raised on the worker thread if injection/initialization fails.</summary>
    public event Action<string>? Error;

    /// <summary>Diagnostic milestones (surfaced to the debug console + optional UI).</summary>
    public event Action<string>? Log;

    private void Trace(string msg)
    {
        // Debug console only (visible in the Visual Studio Output window when the
        // profiler is run under the debugger).
        Debug.WriteLine($"[TapChannel] {DateTime.Now:HH:mm:ss.fff} [pid {_target.Id}] {msg}");
        Log?.Invoke(msg);
    }

    public void Start()
    {
        Trace("Start() requested");
        _worker = new Thread(DoWork) { IsBackground = true, Name = "XamlProfilerTapChannel" };
        _worker.Start();
    }

    private void DoWork()
    {
        try
        {
            if (!CreatePipe(out _readHandle, out _writeHandle, IntPtr.Zero, 4096 * 8))
            {
                Error?.Invoke($"CreatePipe (tap->app) failed: {Marshal.GetLastWin32Error()}");
                return;
            }
            if (!CreatePipe(out _readHandleToTap, out _writeHandleToTap, IntPtr.Zero, 4096 * 8))
            {
                Error?.Invoke($"CreatePipe (app->tap) failed: {Marshal.GetLastWin32Error()}");
                return;
            }

            _fileStreamToTap = new FileStream(_writeHandleToTap, FileAccess.Write);
            _streamWriterToTap = new StreamWriter(_fileStreamToTap, Encoding.Unicode);

            using var pipeClient = new FileStream(_readHandle, FileAccess.Read);

            Process self = Process.GetCurrentProcess();
            DuplicateHandle(self.Handle, _writeHandle!.DangerousGetHandle(),
                _target.Handle, out IntPtr writeHandleForTarget,
                0, false, DUPLICATE_SAME_ACCESS);
            DuplicateHandle(self.Handle, _readHandleToTap!.DangerousGetHandle(),
                _target.Handle, out IntPtr readHandleForTarget,
                0, false, DUPLICATE_SAME_ACCESS);

            string initializationData =
                writeHandleForTarget.ToString("x") + " " + readHandleForTarget.ToString("x");

            string tapDllPath =
                Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "XamlProfilerTap.dll");

            if (!File.Exists(tapDllPath))
            {
                Error?.Invoke($"Tap DLL not found at {tapDllPath}");
                return;
            }

            Trace($"Injecting tap '{tapDllPath}' (clsid {typeof(XamlProfilerTap).GUID:B}) into pid {_target.Id}; initData='{initializationData}'");
            try
            {
                InitializeXamlDiagnosticsEx("WinUIVisualDiagConnection1",
                    _target.Id,
                    "Microsoft.UI.Xaml.dll",
                    tapDllPath,
                    typeof(XamlProfilerTap).GUID,
                    initializationData);
                Trace("InitializeXamlDiagnosticsEx returned OK; waiting for tap 'Connected'...");
            }
            catch (Exception ex)
            {
                Trace($"InitializeXamlDiagnosticsEx FAILED: 0x{ex.HResult:X8} {ex.GetType().Name} {ex.Message}");
                Error?.Invoke($"InitializeXamlDiagnosticsEx failed: 0x{ex.HResult:X8} {ex.Message}");
                return;
            }

            using var sr = new StreamReader(pipeClient, Encoding.Unicode);
            string? line;
            while (!_stopped && (line = sr.ReadLine()) != null)
            {
                TapMessage? msg;
                try { msg = JsonSerializer.Deserialize<TapMessage>(line); }
                catch { continue; }

                if (msg?.TapType == "Connected")
                {
                    if (!string.IsNullOrEmpty(msg.ModuleHandle))
                    {
                        try { _tapModuleHandle = new IntPtr(Convert.ToInt64(msg.ModuleHandle, 16)); }
                        catch { }
                    }
                    _connected = true;
                    Trace($"Tap CONNECTED (module {msg.ModuleHandle}).");
                    Connected?.Invoke();

                    // Flush a highlight requested before the tap finished connecting.
                    ulong pending = _pendingHighlight;
                    if (pending != 0)
                    {
                        _pendingHighlight = 0;
                        Trace($"Flushing queued highlight 0x{pending:X16}");
                        WriteToTap("HIGHLIGHT: " + pending.ToString("X16"));
                    }
                }
                else if (msg?.TapType == "Select")
                {
                    // Pick mode: the tap reported the element the user clicked in the
                    // target app. Handle is "%p" hex (no "0x"), == the node's PeerHandle.
                    if (!string.IsNullOrEmpty(msg.Handle) &&
                        ulong.TryParse(msg.Handle, System.Globalization.NumberStyles.HexNumber,
                            System.Globalization.CultureInfo.InvariantCulture, out ulong picked) &&
                        picked != 0)
                    {
                        Trace($"Tap picked element 0x{picked:X16}");
                        ElementPicked?.Invoke(picked);
                    }

                    // The tap also reports the clicked element's composition visual id
                    // (xpid stamp or raw IVisual*). The profiler uses it to glow that
                    // visual node's whole subtree in the IVisual/Composition tree.
                    if (!string.IsNullOrEmpty(msg.VisualId) &&
                        ulong.TryParse(msg.VisualId, System.Globalization.NumberStyles.HexNumber,
                            System.Globalization.CultureInfo.InvariantCulture, out ulong pickedVisual) &&
                        pickedVisual != 0)
                    {
                        Trace($"Tap picked visual 0x{pickedVisual:X16}");
                        VisualPicked?.Invoke(pickedVisual);
                    }
                }
                else if (msg?.TapType == "Info")
                {
                    // Free-text diagnostic line from the tap (e.g. the responsible visual
                    // for a picked element); surface it in the profiler log.
                    if (!string.IsNullOrEmpty(msg.Text))
                        Trace(msg.Text);
                }
                // VisualTreeChange / ElementProperties / etc. are drained but ignored;
                // we only need the one-way highlight command channel.
            }
        }
        catch (Exception ex)
        {
            if (!_stopped)
                Error?.Invoke(ex.Message);
        }
    }

    /// <summary>Highlight the live element whose XAML-diagnostics handle == <paramref name="peerHandle"/>.</summary>
    public void Highlight(ulong peerHandle)
    {
        if (peerHandle == 0)
            return;
        // Match MSVC swprintf_s(L"%p") so the tap's swscanf_s(L"HIGHLIGHT: %p") round-trips:
        // 64-bit pointer => 16 upper-hex digits, no "0x" prefix.
        if (!_connected)
        {
            // Tap not connected yet (e.g. first click right after session start).
            // Remember it and send as soon as the tap reports "Connected".
            _pendingHighlight = peerHandle;
            Trace($"Highlight 0x{peerHandle:X16} queued (tap not connected yet)");
            return;
        }
        WriteToTap("HIGHLIGHT: " + peerHandle.ToString("X16"));
    }

    public void ClearHighlight() => WriteToTap("CLEAR-HIGHLIGHT");

    /// <summary>Enter pick mode: hovering the target app previews the element under the
    /// cursor; a click selects it (raising <see cref="ElementPicked"/>) and exits pick mode.</summary>
    public void StartPick() => WriteToTap("START-PICK");

    /// <summary>Leave pick mode.</summary>
    public void StopPick() => WriteToTap("STOP-PICK");

    /// <summary>
    /// Highlight a live IVisual / Composition-only node (no XAML peer handle). The
    /// producer stamps each live visual's Comment with "xpid:&lt;IVisual* hex&gt;", which
    /// equals this node's Id; the tap walks the live composition tree to find and adorn it.
    /// </summary>
    public void HighlightVisual(ulong visualId)
    {
        if (visualId == 0)
            return;
        // Match the tap's swscanf_s(L"HIGHLIGHT-VISUAL: %llx"): 16 upper-hex digits, no "0x".
        WriteToTap("HIGHLIGHT-VISUAL: " + visualId.ToString("X16"));
    }

    private void WriteToTap(string s)
    {
        lock (_writeLock)
        {
            try
            {
                _streamWriterToTap?.WriteLine(s);
                _streamWriterToTap?.Flush();
                Trace($"SENT command: \"{s}\"");
            }
            catch (Exception ex)
            {
                Trace($"SEND FAILED for \"{s}\": {ex.Message}");
            }
        }
    }

    public void Stop()
    {
        if (_stopped)
            return;
        _stopped = true;

        lock (_writeLock)
        {
            try { _streamWriterToTap?.WriteLine("CLOSE"); _streamWriterToTap?.Flush(); }
            catch { }
        }

        // Give the tap a moment to tear down its overlay/threads.
        Thread.Sleep(1000);

        lock (_writeLock)
        {
            try { _streamWriterToTap?.Dispose(); } catch { }
            _streamWriterToTap = null;
            try { _fileStreamToTap?.Dispose(); } catch { }
            _fileStreamToTap = null;
        }

        try { _readHandle?.Close(); } catch { }
        try { _writeHandle?.Close(); } catch { }

        EjectTapDll();

        _connected = false;
    }

    private void EjectTapDll()
    {
        if (_tapModuleHandle == IntPtr.Zero)
            return;

        try
        {
            if (_target.HasExited)
                return;

            IntPtr hProcess = OpenProcess(
                PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                false, _target.Id);
            if (hProcess == IntPtr.Zero)
                return;

            try
            {
                IntPtr kernel32 = GetModuleHandle("kernel32.dll");
                IntPtr freeLibraryAddr = GetProcAddress(kernel32, "FreeLibrary");
                if (freeLibraryAddr == IntPtr.Zero)
                    return;

                for (int i = 0; i < 5; i++)
                {
                    IntPtr hThread = CreateRemoteThread(hProcess, IntPtr.Zero, UIntPtr.Zero,
                        freeLibraryAddr, _tapModuleHandle, 0, out _);
                    if (hThread == IntPtr.Zero)
                        break;

                    WaitForSingleObject(hThread, 5000);
                    GetExitCodeThread(hThread, out uint exitCode);
                    CloseHandle(hThread);

                    if (exitCode == 0)
                        break; // FreeLibrary returned FALSE => fully unloaded
                }
            }
            finally
            {
                CloseHandle(hProcess);
            }
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"[TapChannel] EjectTapDll exception: {ex.Message}");
        }
    }

    public void Dispose() => Stop();

    private sealed class TapMessage
    {
        public string? TapType { get; set; }
        public string? ModuleHandle { get; set; }
        public string? Handle { get; set; }
        public string? VisualId { get; set; }
        public string? Text { get; set; }
    }

    // ---- P/Invoke (verbatim from WinUISnoop TapHandler.cs) ----

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool CreatePipe(out SafeFileHandle hReadPipe, out SafeFileHandle hWritePipe, IntPtr lpPipeAttributes, uint nSize);

    [DllImport("Microsoft.Internal.FrameworkUDK.dll", CharSet = CharSet.Unicode, PreserveSig = false)]
    private static extern void InitializeXamlDiagnosticsEx(string endPointName, int pid, string dllXamlDiagnostics, string tapDllname, Guid tapClsid, string initializationData);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool DuplicateHandle(IntPtr hSourceProcessHandle,
        IntPtr hSourceHandle, IntPtr hTargetProcessHandle, out IntPtr lpTargetHandle,
        uint dwDesiredAccess, bool bInheritHandle, uint dwOptions);

    private const uint DUPLICATE_SAME_ACCESS = 0x00000002;

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes,
        UIntPtr dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter,
        uint dwCreationFlags, out uint lpThreadId);

    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true)]
    private static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr GetModuleHandle(string lpModuleName);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GetExitCodeThread(IntPtr hThread, out uint lpExitCode);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool CloseHandle(IntPtr hObject);

    private const uint PROCESS_CREATE_THREAD = 0x0002;
    private const uint PROCESS_QUERY_INFORMATION = 0x0400;
    private const uint PROCESS_VM_OPERATION = 0x0008;
    private const uint PROCESS_VM_WRITE = 0x0020;
    private const uint PROCESS_VM_READ = 0x0010;
}

// Declares the CLSID of the native tap implemented in XamlProfilerTap.dll.
[Guid("c9f2ef77-1b6a-4810-a490-6ea6a06bf7cb")]
internal sealed class XamlProfilerTap { }
