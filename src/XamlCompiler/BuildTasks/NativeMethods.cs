// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;
    using XBF;

    internal static class NativeMethodsHelper
    {
        private static bool s_VcMetaIsLoaded = false;
        public static bool EnsureVcMetaIsLoaded(string vsInstallDir, string vcInstallPath32, string vcInstallPath64)
        {
            if (!s_VcMetaIsLoaded)
            {
                // First, try the direct paths.  VS may not have supplied us with them so fall back to the old vsInstallDir logic if necessary.
                string path32 = vcInstallPath32;
                string path64 = vcInstallPath64;
                if (LoadLibrary32_64(path32, path64) != IntPtr.Zero)
                {
                    s_VcMetaIsLoaded = true;
                }
                else if (!string.IsNullOrEmpty(vsInstallDir))
                {
                    // The direct paths we were given were invalid - fall back to the vsInstallDir logic by checking the hardcoded paths
                    // relative to the install directory.

                    // First, check the old hardcoded layout
                    path32 = Path.Combine(vsInstallDir, @"bin\VcMeta.dll");
                    path64 = Path.Combine(vsInstallDir, @"bin\amd64\VcMeta.dll");
                    if (LoadLibrary32_64(path32, path64) != IntPtr.Zero)
                    {
                        s_VcMetaIsLoaded = true;
                    }
                    else
                    {
                        // The layout changed locations in recent VS versions, and we only have access to the 32-bit vcmeta.dll in this case.
                        // If this is 64-bit, we're guaranteed to fail again here as the 64-bit version isn't present - just reuse
                        // the 64-bit path we already tried and accept our fate
                        path32 = Path.Combine(vsInstallDir, @"vcpackages\vcmeta.dll");
                        if (LoadLibrary32_64(path32, path64) != IntPtr.Zero)
                        {
                            s_VcMetaIsLoaded = true;
                        }
                    }
                }
            }
            return s_VcMetaIsLoaded;
        }

        /// <summary>
        /// Loads the dll based on the process architecture and returns the dll handle
        /// </summary>
        /// <param name="path32bit">Path to the 32 bit binary</param>
        /// <param name="path64bit">Path to the 64 bit binary</param>
        /// <returns>Returns the Dll Handle</returns>
        private static IntPtr LoadLibrary32_64(string path32bit, string path64bit)
        {
            string dllPath = (System.Environment.Is64BitProcess) ? path64bit : path32bit;
            return NativeMethods.LoadLibraryEx(dllPath, IntPtr.Zero, NativeMethods.LOAD_WITH_ALTERED_SEARCH_PATH);
        }

        public static int Write(IntPtr dllHandle, IStream[] xamlStreams, int numFiles, string[] pbChecksum, int checksumSize, 
            IXbfMetadataProvider provider, TargetOSVersion targetVersion, uint xbfGenerationFlags, IStream[] xbfStreams,
            out int errorCode, out int errorFileIndex, out int errorLine, out int errorColumn)
        {
            if (dllHandle == IntPtr.Zero)
            {
                throw new ArgumentException("dllHandle");
            }

            IntPtr writeAddress = NativeMethods.GetProcAddress(dllHandle, "Write");
            if (writeAddress != IntPtr.Zero)
            {
                NativeMethods.Write write = (NativeMethods.Write)Marshal.GetDelegateForFunctionPointer(writeAddress, typeof(NativeMethods.Write));
                return write(xamlStreams, numFiles, pbChecksum, checksumSize, provider, targetVersion, xbfGenerationFlags, xbfStreams, out errorCode, out errorFileIndex, out errorLine, out errorColumn);
            }

            System.Diagnostics.Debug.Fail("Failed to correctly call GenXbf::Write. This should never happen!");
            errorCode = -1;
            errorFileIndex = -1;
            errorLine = -1;
            errorColumn = -1;
            return -1;
        }
    }
    internal static class NativeMethods
    {
        public static int LOAD_WITH_ALTERED_SEARCH_PATH = 0x00000008;

        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode, EntryPoint = "LoadLibraryEx")]
        public static extern IntPtr LoadLibraryEx(string libFilename, IntPtr reserved, int flags);

        [DllImport("kernel32", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
        public static extern int FreeLibrary(IntPtr hModule);

        [DllImport("vcmeta.dll", CharSet = CharSet.Auto, EntryPoint = "HashForWinMD", CallingConvention = CallingConvention.Cdecl)]
        public static extern int HashForWinMD(String lpFileName, out Guid hash);

        public delegate int Write(
            IStream[] xamlStreams, int numFiles, string[] pbChecksum, int checksumSize, 
            IXbfMetadataProvider provider, TargetOSVersion targetVersion, uint xbfGenerationFlags, IStream[] xbfStreams,
            out int errorCode, out int errorFileIndex, out int errorLine, out int errorColumn);
    }
}
