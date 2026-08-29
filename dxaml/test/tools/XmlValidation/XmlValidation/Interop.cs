// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace XmlValidation
{
    internal static class Interop
    {
        [System.Flags]
        enum LoadLibraryFlags : uint
        {
            None = 0,
            DONT_RESOLVE_DLL_REFERENCES = 0x00000001,
            LOAD_IGNORE_CODE_AUTHZ_LEVEL = 0x00000010,
            LOAD_LIBRARY_AS_DATAFILE = 0x00000002,
            LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE = 0x00000040,
            LOAD_LIBRARY_AS_IMAGE_RESOURCE = 0x00000020,
            LOAD_LIBRARY_SEARCH_APPLICATION_DIR = 0x00000200,
            LOAD_LIBRARY_SEARCH_DEFAULT_DIRS = 0x00001000,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR = 0x00000100,
            LOAD_LIBRARY_SEARCH_SYSTEM32 = 0x00000800,
            LOAD_LIBRARY_SEARCH_USER_DIRS = 0x00000400,
            LOAD_WITH_ALTERED_SEARCH_PATH = 0x00000008
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hReservedNull, LoadLibraryFlags dwFlags);

        const int TESTRESOURCE = 256;

        [DllImport("Kernel32.dll", EntryPoint = "FindResourceW", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern IntPtr FindResource(IntPtr hModule, string pName, int type);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint SizeofResource(IntPtr hModule, IntPtr hResInfo);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr LoadResource(IntPtr hModule, IntPtr hResInfo);

        [DllImport("kernel32.dll")]
        private static extern IntPtr LockResource(IntPtr hResData);

        private static IntPtr resourcesModuleHandle = IntPtr.Zero;

        public static byte[] ReadTestResource(string resourceName)
        {
            if (resourcesModuleHandle == IntPtr.Zero)
            {
                resourcesModuleHandle = LoadLibraryEx("Private.Infrastructure.Resources.dll", IntPtr.Zero, LoadLibraryFlags.LOAD_LIBRARY_AS_DATAFILE | LoadLibraryFlags.LOAD_LIBRARY_AS_IMAGE_RESOURCE);
            }

            if (resourcesModuleHandle == IntPtr.Zero)
            {
                throw new InvalidOperationException($"GLE:{Marshal.GetLastWin32Error()}");
            }

            var rc = FindResource(resourcesModuleHandle, resourceName, TESTRESOURCE);
            if (rc == IntPtr.Zero)
            {
                throw new InvalidOperationException($"GLE:{Marshal.GetLastWin32Error()}");
            }

            var dataLength = SizeofResource(resourcesModuleHandle, rc);
            if (0 == dataLength)
            {
                throw new InvalidOperationException($"GLE:{Marshal.GetLastWin32Error()}");
            }

            var rcData = LoadResource(resourcesModuleHandle, rc);
            if (rcData == IntPtr.Zero)
            {
                throw new InvalidOperationException($"GLE:{Marshal.GetLastWin32Error()}");
            }

            var data = LockResource(rcData);
            byte[] buffer = new byte[dataLength];
            Marshal.Copy(data, buffer, 0, (int)dataLength);
            return buffer;
        }
    }
}
