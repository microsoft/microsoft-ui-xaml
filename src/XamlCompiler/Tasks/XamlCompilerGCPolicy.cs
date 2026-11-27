// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    /// <summary>Chooses the GC startup settings of the native XAML compiler, without changing the MSBuild process environment.</summary>
    internal static class XamlCompilerGCPolicy
    {
        /// <summary>The minimum processor count for automatic Server GC selection.</summary>
        private const int MinimumProcessorCount = 4;

        /// <summary>The minimum system memory in megabytes for automatic Server GC selection.</summary>
        private const ulong MinimumSystemMemoryInMegabytes = 16 * 1024;

        // Declared by hand: this assembly is netstandard2.0, for which CsWin32 would generate LibraryImport code that doesn't compile
        [DllImport("kernel32.dll", ExactSpelling = true, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetPhysicallyInstalledSystemMemory(out ulong totalMemoryInKilobytes);

        /// <summary>Determines whether this system should use Server GC, based on the processor count and the installed physical memory.</summary>
        public static bool ShouldUseServerGC()
        {
            // Server GC trades dedicated threads and larger heaps for throughput
            int processors = Environment.ProcessorCount;

            // Check for enough system RAM to accommodate the higher peak memory usage with Server GC
            if (!GetPhysicallyInstalledSystemMemory(out ulong memoryInKilobytes))
            {
                // Without a memory estimate, keep the conservative workstation GC default
                return false;
            }

            ulong memoryInMegabytes = memoryInKilobytes / 1024;

            return processors >= MinimumProcessorCount && memoryInMegabytes >= MinimumSystemMemoryInMegabytes;
        }
    }
}
