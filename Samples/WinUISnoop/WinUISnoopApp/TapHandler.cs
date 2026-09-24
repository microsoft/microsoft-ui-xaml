using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Win32.SafeHandles;
//using ABI.Windows.Data.Json;
using System.Text.Json;
using System.Diagnostics;
using Microsoft.UI.Dispatching;


namespace WinUISnoopApp
{
    class TapHandler
    {
        public TapHandler(MainWindow mainWindow, Process connectProcess)
        {
            this.ConnectProcess = connectProcess;
            this.mainWindow = mainWindow;
            this.dispatcherQueue = mainWindow.DispatcherQueue;
        }

        public void DoWork()
        {
            this.thread = Thread.CurrentThread;

            if (!CreatePipe(out readHandle, out writeHandle, IntPtr.Zero, 4096 * 8))
            {
                var err = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
                System.Diagnostics.Debugger.Break();
                return;
            }
            if (!CreatePipe(out readHandleToTap, out writeHandleToTap, IntPtr.Zero, 4096 * 8))
            {
                var err = System.Runtime.InteropServices.Marshal.GetLastWin32Error();
                System.Diagnostics.Debugger.Break();
                return;
            }
            fileStreamToTap = new FileStream(writeHandleToTap, FileAccess.Write);
            streamWriterToTap = new StreamWriter(fileStreamToTap, System.Text.Encoding.Unicode);

            using (FileStream pipeClient = new FileStream(readHandle, FileAccess.Read))
            {
                Console.WriteLine("[CLIENT] started");

                // Duplicate the read/write handles for the target and encode the handles into
                // the initializationData string.
                Process snoopProcess = Process.GetCurrentProcess();
                IntPtr writeHandleForTarget;
                IntPtr readHandleForTarget;
                DuplicateHandle(snoopProcess.Handle, writeHandle.DangerousGetHandle(),
                    this.ConnectProcess.Handle, out writeHandleForTarget,
                    0, false, DUPLICATE_SAME_ACCESS);
                DuplicateHandle(snoopProcess.Handle, readHandleToTap.DangerousGetHandle(),
                    this.ConnectProcess.Handle, out readHandleForTarget,
                    0, false, DUPLICATE_SAME_ACCESS);

                string initializationData = "";
                initializationData =
                    writeHandleForTarget.ToString("x") +
                    " " +
                   readHandleForTarget.ToString("x");

                string tapDllPath =
                    Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "WinUISnoopTap.dll");

                InitializeXamlDiagnosticsEx("WinUIVisualDiagConnection1",
                    this.ConnectProcess.Id,
                    "Microsoft.UI.Xaml.dll",
                    tapDllPath,
                    typeof(XamlSnoopTap).GUID,
                    initializationData);

                using (StreamReader sr = new StreamReader(pipeClient, System.Text.Encoding.Unicode))
                {
                    // Display the read text to the console
                    string temp;

                    // Read the server data and echo to the console.
                    while ((temp = sr.ReadLine()) != null)
                    {
                        //System.Diagnostics.Debug.WriteLine("[CLIENT] Echo: " + temp);
                        var tapType = JsonSerializer.Deserialize<TAPType>(temp);
                        if (tapType.TapType == "Connected")
                        {
                            // Store the module handle for DLL ejection on Stop
                            if (!string.IsNullOrEmpty(tapType.ModuleHandle))
                            {
                                try
                                {
                                    tapModuleHandle = new IntPtr(Convert.ToInt64(tapType.ModuleHandle, 16));
                                    Debug.WriteLine($"[SNOOP] Tap module handle: {tapType.ModuleHandle}");
                                }
                                catch { }
                            }
                            this.dispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                            {
                                mainWindow.HandleConnected();
                            }));
                        }
                        else if (tapType.TapType == "VisualTreeChange")
                        {
                            var visualTreeChange = JsonSerializer.Deserialize<TAPVisualTreeChange>(temp);

                            this.dispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                            {
                                mainWindow.HandleVisualTreeChange(visualTreeChange);
                            }));
                        }
                        else if (tapType.TapType == "ElementProperties")
                        {
                            var elementProperties = JsonSerializer.Deserialize<TAPElementProperties>(temp);

                            this.dispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                            {
                                mainWindow.HandleElementProperties(elementProperties);
                            }));
                        }
                        else if (tapType.TapType == "SubTreePropertiesDone")
                        {
                            var msg = JsonSerializer.Deserialize<TAPSubTreeDone>(temp);

                            this.dispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
                            {
                                mainWindow.HandleSubTreePropertiesDone(msg.Element);
                            }));
                        }
                        else
                        {
                            System.Diagnostics.Debug.WriteLine("[CLIENT] Echo unknown data: " + temp);
                        }
                    }
                }
            }
        }

        [DllImport("kernel32.dll")]
        public static extern bool CreatePipe(out SafeFileHandle hReadPipe, out SafeFileHandle hWritePipe, IntPtr lpPipeAttributes, uint nSize);

        [DllImport("Microsoft.Internal.FrameworkUDK.dll", CharSet = CharSet.Unicode, PreserveSig=false)]
        public static extern void InitializeXamlDiagnosticsEx(string endPointName, int pid, string dllXamlDiagnostics, string tapDllname, Guid tapClsid, string initializationData);


        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool DuplicateHandle(IntPtr hSourceProcessHandle,
           IntPtr hSourceHandle, IntPtr hTargetProcessHandle, out IntPtr lpTargetHandle,
           uint dwDesiredAccess, bool bInheritHandle, uint dwOptions);
        private uint DUPLICATE_SAME_ACCESS = 0x00000002; //Ignores the dwDesiredAccess parameter. The duplicate handle has the same access as the source handle.

        public void WriteToTap(string s)
        {
            if (streamWriterToTap != null)
            {
                streamWriterToTap.WriteLine(s);
                streamWriterToTap.Flush();
            }
        }

        public void Start()
        {
            Thread thread = new Thread(new ThreadStart(this.DoWork));
            thread.Start();
        }

        public void Stop()
        {
            if (streamWriterToTap != null)
            {
                WriteToTap("CLOSE");
            }

            // Wait for the tap to finish its cleanup
            Thread.Sleep(1000);

            readHandle.Close();
            writeHandle.Close();

            streamWriterToTap.Close();
            streamWriterToTap.Dispose();
            streamWriterToTap = null;

            fileStreamToTap.Close();
            fileStreamToTap.Dispose();
            fileStreamToTap = null;

            // Eject the tap DLL from the target process using CreateRemoteThread + FreeLibrary
            EjectTapDll();
        }

        private void EjectTapDll()
        {
            if (tapModuleHandle == IntPtr.Zero || ConnectProcess == null)
                return;

            try
            {
                if (ConnectProcess.HasExited)
                    return;

                IntPtr hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                    false, ConnectProcess.Id);
                if (hProcess == IntPtr.Zero)
                {
                    Debug.WriteLine("[SNOOP] EjectTapDll: OpenProcess failed");
                    return;
                }

                try
                {
                    // Get FreeLibrary address from kernel32 — same in all processes (ASLR is per-boot, not per-process)
                    IntPtr kernel32 = GetModuleHandle("kernel32.dll");
                    IntPtr freeLibraryAddr = GetProcAddress(kernel32, "FreeLibrary");
                    if (freeLibraryAddr == IntPtr.Zero)
                    {
                        Debug.WriteLine("[SNOOP] EjectTapDll: GetProcAddress(FreeLibrary) failed");
                        return;
                    }

                    // Call FreeLibrary in the target process up to 5 times to drain all LoadLibrary refs
                    for (int i = 0; i < 5; i++)
                    {
                        IntPtr hThread = CreateRemoteThread(hProcess, IntPtr.Zero, UIntPtr.Zero,
                            freeLibraryAddr, tapModuleHandle, 0, out _);
                        if (hThread == IntPtr.Zero)
                        {
                            Debug.WriteLine($"[SNOOP] EjectTapDll: CreateRemoteThread failed (attempt {i + 1})");
                            break;
                        }

                        // Wait for the remote FreeLibrary call to complete
                        WaitForSingleObject(hThread, 5000);

                        // Check the return value (FreeLibrary returns BOOL — nonzero = success)
                        GetExitCodeThread(hThread, out uint exitCode);
                        CloseHandle(hThread);

                        Debug.WriteLine($"[SNOOP] EjectTapDll: FreeLibrary attempt {i + 1}, result = {exitCode}");

                        if (exitCode == 0)
                        {
                            // FreeLibrary returned FALSE — module was already unloaded or handle invalid
                            Debug.WriteLine("[SNOOP] EjectTapDll: DLL successfully ejected");
                            break;
                        }
                    }
                }
                finally
                {
                    CloseHandle(hProcess);
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"[SNOOP] EjectTapDll exception: {ex.Message}");
            }
        }

        private IntPtr tapModuleHandle = IntPtr.Zero;

        public Process ConnectProcess { get; }
        private Thread thread;
        private MainWindow mainWindow;
        private DispatcherQueue dispatcherQueue;

        // Tap to Snoop handles
        private SafeFileHandle readHandle;
        private SafeFileHandle writeHandle;

        // Snoop to Tap handles
        private SafeFileHandle readHandleToTap;
        private SafeFileHandle writeHandleToTap;
        private FileStream fileStreamToTap;
        private StreamWriter streamWriterToTap;

        // P/Invoke for DLL ejection
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes,
            UIntPtr dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter,
            uint dwCreationFlags, out uint lpThreadId);

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true)]
        static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool GetExitCodeThread(IntPtr hThread, out uint lpExitCode);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);

        const uint PROCESS_CREATE_THREAD = 0x0002;
        const uint PROCESS_QUERY_INFORMATION = 0x0400;
        const uint PROCESS_VM_OPERATION = 0x0008;
        const uint PROCESS_VM_WRITE = 0x0020;
        const uint PROCESS_VM_READ = 0x0010;
    }

    // Declare the class and Guid of XamlSnoopTap, implemented in the native WinUISnoopTap.dll
    [Guid("e6755080-f13e-4864-931e-06368e30a84a")]
    class XamlSnoopTap { }

}
