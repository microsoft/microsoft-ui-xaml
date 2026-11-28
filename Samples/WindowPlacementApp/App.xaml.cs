// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Linq;
using Microsoft.UI.Xaml;

namespace WindowPlacementApp
{
    public partial class App : Application
    {
        public App()
        {
            InitializeComponent();
            UnhandledException += (sender, e) => Log.Write($"UNHANDLED: 0x{e.Exception.HResult:X8} {e.Message}");
        }

        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            string[] commandLine = Environment.GetCommandLineArgs().Skip(1).ToArray();
            LaunchOptions options = LaunchOptions.FromCommandLine(commandLine);
            Log.Write($"Process started. Args: \"{string.Join(" ", commandLine)}\". Package identity: {Log.PackageIdentity()}");
            Log.Write(DescribeStartupInfo());
            PlacementWindow.Open(options, isMain: true);
        }

        // A window placed with isFirstWindow = true honors STARTUPINFO, so log what the launcher passed.
        // Only the SaveOnClose and Manual setups pass true (for the main window). RestoreAndSave always passes false.
        private static string DescribeStartupInfo()
        {
            var si = new STARTUPINFOW { cb = System.Runtime.InteropServices.Marshal.SizeOf<STARTUPINFOW>() };
            GetStartupInfoW(ref si);
            return $"STARTUPINFO: dwFlags=0x{si.dwFlags:X} wShowWindow={si.wShowWindow} pos=({si.dwX},{si.dwY}) size={si.dwXSize}x{si.dwYSize} hStdOutput=0x{(long)si.hStdOutput:X}";
        }

        [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        private struct STARTUPINFOW
        {
            public int cb;
            public IntPtr lpReserved;
            public IntPtr lpDesktop;
            public IntPtr lpTitle;
            public int dwX;
            public int dwY;
            public int dwXSize;
            public int dwYSize;
            public int dwXCountChars;
            public int dwYCountChars;
            public int dwFillAttribute;
            public int dwFlags;
            public short wShowWindow;
            public short cbReserved2;
            public IntPtr lpReserved2;
            public IntPtr hStdInput;
            public IntPtr hStdOutput;
            public IntPtr hStdError;
        }

        [System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
        private static extern void GetStartupInfoW(ref STARTUPINFOW si);
    }
}
