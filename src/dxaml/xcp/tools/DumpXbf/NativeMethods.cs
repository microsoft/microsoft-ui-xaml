// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DumpXBF
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;

    internal static class NativeMethodsHelper
    {
        private static bool s_GenXbfIsLoaded = false;

        public static bool GenXbfIsLoaded
        {
            get { return s_GenXbfIsLoaded; }
        }

        public static bool EnsureGenXbfIsLoaded(string genXbfPath)
        {
            if (genXbfPath == null)
            {
                throw new ArgumentNullException("genXbfPath");
            }
            if (!s_GenXbfIsLoaded)
            {
                string path = Path.Combine(genXbfPath, @"genxbf.dll");
                if (LoadLibraryWithPath(path))
                {
                    s_GenXbfIsLoaded = true;
                }
            }
            return s_GenXbfIsLoaded;
        }

        private static bool LoadLibraryWithPath(string libFilePath)
        {
            IntPtr ret = NativeMethods.LoadLibraryEx(libFilePath, IntPtr.Zero, NativeMethods.LOAD_WITH_ALTERED_SEARCH_PATH);
            return (ret != IntPtr.Zero);
        }

    }
    internal static class NativeMethods
    {
        internal static int LOAD_WITH_ALTERED_SEARCH_PATH = 0x00000008;

        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode, EntryPoint = "LoadLibraryEx")]
        internal static extern IntPtr LoadLibraryEx(string libFilename, IntPtr reserved, int flags);

        [DllImportAttribute("GenXbf.dll", CallingConvention = CallingConvention.Winapi, EntryPoint = "Dump", CharSet = CharSet.Unicode)]
        public static extern int Dump(IStream xbfStream, IStream outputStream, int mode, out int errorCode);
    }

}
