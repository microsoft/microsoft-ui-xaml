// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Native AOT (net10.0) only: the native interop used by the XAML compiler, without built-in COM interop or runtime
// marshalling (both unavailable with Native AOT and DisableRuntimeMarshalling). Managed objects are passed to native
// code as source-generated COM objects, and native exports are called through blittable unmanaged function pointers.

namespace Microsoft.UI.Xaml.Markup.Compiler.Utilities
{
    using System;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;
    using System.Runtime.InteropServices.Marshalling;
    using FileIO;
    using Windows.Win32;
    using Windows.Win32.Foundation;
    using Windows.Win32.System.LibraryLoader;
    using XBF;

    internal static unsafe partial class NativeMethodsHelper
    {
        public static int Write(IntPtr dllHandle, IStream[] xamlStreams, int numFiles, string[] pbChecksum, int checksumSize,
            IXbfMetadataProvider provider, TargetOSVersion targetVersion, uint xbfGenerationFlags, IStream[] xbfStreams,
            out int errorCode, out int errorFileIndex, out int errorLine, out int errorColumn)
        {
            if (dllHandle == IntPtr.Zero)
            {
                throw new ArgumentException("dllHandle");
            }

            IntPtr writeAddress = NativeMethods.GetProcAddress(dllHandle, "Write");
            if (writeAddress == IntPtr.Zero)
            {
                System.Diagnostics.Debug.Fail("Failed to correctly call GenXbf::Write. This should never happen!");
                errorCode = -1;
                errorFileIndex = -1;
                errorLine = -1;
                errorColumn = -1;
                return -1;
            }

            // extern "C" HRESULT WINAPI Write(
            //     IStream** ppXamlStreams, UINT32 numFiles, byte** pbChecksum, UINT32 cbChecksum, IXbfMetadataProvider* pMetadataProvider,
            //     const TargetOSVersion& targetOS, XbfGenerationFlags generatorFlags, IStream** ppXbfStreams,
            //     UINT32* puiErrorCode, UINT32* puiErrorFileIndex, UINT32* puiErrorLine, UINT32* puiErrorColumn)
            // See dxaml\xcp\tools\GenXbfDLL\Main.cpp.
            var write = (delegate* unmanaged[Stdcall]<void**, uint, byte**, uint, void*, NativeTargetOSVersion*, uint, void**, int*, int*, int*, int*, int>)writeAddress;

            // The native code only writes the error out parameters on failure, so they need a defined initial value
            errorCode = 0;
            errorFileIndex = 0;
            errorLine = 0;
            errorColumn = 0;

            void** xamlStreamPointers = null;
            void** xbfStreamPointers = null;
            byte** checksumPointers = null;
            void* providerPointer = null;
            try
            {
                CreateStreamPointers(xamlStreams, out xamlStreamPointers);
                CreateStreamPointers(xbfStreams, out xbfStreamPointers);
                CreateChecksumPointers(pbChecksum, checksumSize, out checksumPointers);
                providerPointer = provider == null ? null : ComInterfaceMarshaller<IXbfMetadataProviderNative>.ConvertToUnmanaged((IXbfMetadataProviderNative)provider);

                NativeTargetOSVersion nativeTargetVersion = default;
                if (targetVersion != null)
                {
                    nativeTargetVersion.major = targetVersion.major;
                    nativeTargetVersion.minor = targetVersion.minor;
                    nativeTargetVersion.build = targetVersion.build;
                    nativeTargetVersion.revision = targetVersion.revision;
                }

                fixed (int* pErrorCode = &errorCode)
                fixed (int* pErrorFileIndex = &errorFileIndex)
                fixed (int* pErrorLine = &errorLine)
                fixed (int* pErrorColumn = &errorColumn)
                {
                    return write(
                        xamlStreamPointers,
                        (uint)numFiles,
                        checksumPointers,
                        (uint)checksumSize,
                        providerPointer,
                        targetVersion == null ? null : &nativeTargetVersion,
                        xbfGenerationFlags,
                        xbfStreamPointers,
                        pErrorCode,
                        pErrorFileIndex,
                        pErrorLine,
                        pErrorColumn);
                }
            }
            finally
            {
                ComInterfaceMarshaller<IXbfMetadataProviderNative>.Free(providerPointer);
                FreeStreamPointers(xamlStreamPointers, xamlStreams);
                FreeStreamPointers(xbfStreamPointers, xbfStreams);
                FreeChecksumPointers(checksumPointers, pbChecksum);
            }
        }

        // The out parameters are assigned before anything can fail, so that the caller can always release what was created
        private static void CreateStreamPointers(IStream[] streams, out void** pointers)
        {
            pointers = null;
            if (streams == null)
            {
                return;
            }

            pointers = (void**)NativeMemory.AllocZeroed((nuint)Math.Max(streams.Length, 1), (nuint)sizeof(void*));
            for (int i = 0; i < streams.Length; i++)
            {
                // All the streams are StreamImpl instances (see XbfGenerator.GetInputOutputStreams), which implement the COM IStream
                pointers[i] = streams[i] == null ? null : ComInterfaceMarshaller<Windows.Win32.System.Com.IStream>.ConvertToUnmanaged((Windows.Win32.System.Com.IStream)(object)streams[i]);
                            }
                        }

        private static void FreeStreamPointers(void** pointers, IStream[] streams)
        {
            if (pointers == null)
            {
                return;
            }

            for (int i = 0; i < streams.Length; i++)
            {
                ComInterfaceMarshaller<Windows.Win32.System.Com.IStream>.Free(pointers[i]);
            }
            NativeMemory.Free(pointers);
        }

        // The native code copies exactly checksumSize bytes from each checksum (Main.cpp), which the built-in marshalling
        // provided as ANSI strings. The checksums are hex strings (see ChecksumHelper), so they are passed as ASCII here, in
        // zero-initialized buffers that are always at least checksumSize bytes long and NUL-terminated.
        private static void CreateChecksumPointers(string[] checksums, int checksumSize, out byte** pointers)
        {
            pointers = null;
            if (checksums == null)
            {
                return;
            }

            pointers = (byte**)NativeMemory.AllocZeroed((nuint)Math.Max(checksums.Length, 1), (nuint)sizeof(byte*));
            for (int i = 0; i < checksums.Length; i++)
            {
                string checksum = checksums[i];
                if (checksum == null)
                {
                    continue;
                }

                byte* buffer = (byte*)NativeMemory.AllocZeroed((nuint)Math.Max(checksum.Length + 1, checksumSize));
                for (int j = 0; j < checksum.Length; j++)
                {
                    char c = checksum[j];
                    buffer[j] = c <= 0x7F ? (byte)c : (byte)'?';
                }
                pointers[i] = buffer;
            }
        }

        private static void FreeChecksumPointers(byte** pointers, string[] checksums)
        {
            if (pointers == null)
            {
                return;
            }

            for (int i = 0; i < checksums.Length; i++)
            {
                NativeMemory.Free(pointers[i]);
            }
            NativeMemory.Free(pointers);
        }

        /// <summary>
        /// Blittable equivalent of the native <c>TargetOSVersion</c> struct (dxaml\xcp\components\parser\inc\XbfVersioning.h),
        /// with the same layout as the managed <see cref="TargetOSVersion"/> class.
        /// </summary>
        [StructLayout(LayoutKind.Sequential)]
        private struct NativeTargetOSVersion
        {
            public ushort major;
            public ushort minor;
            public ushort build;
            public ushort revision;
        }
    }

    internal static unsafe partial class NativeMethods
    {
        /// <summary>
        /// The handle of the vcmeta.dll module loaded by <see cref="NativeMethodsHelper.EnsureVcMetaIsLoaded"/>, if any.
        /// </summary>
        internal static IntPtr VcMetaHandle;

        // The P/Invokes are generated by CsWin32 (see NativeMethods.txt in the Exe project); these keep the signatures that the shared code uses
        public static IntPtr LoadLibraryEx(string libFilename, IntPtr reserved, int flags)
        {
            fixed (char* pFileName = libFilename)
            {
                return PInvoke.LoadLibraryEx(new PCWSTR(pFileName), new HANDLE((void*)reserved), (LOAD_LIBRARY_FLAGS)flags);
            }
        }

        public static IntPtr GetProcAddress(IntPtr hModule, string procName)
        {
            // The exported names are ASCII
            byte* name = stackalloc byte[procName.Length + 1];
            for (int i = 0; i < procName.Length; i++)
            {
                name[i] = (byte)procName[i];
            }
            name[procName.Length] = 0;
            return (IntPtr)PInvoke.GetProcAddress(new HMODULE((void*)hModule), new PCSTR(name)).Value;
        }

        public static int FreeLibrary(IntPtr hModule)
        {
            return PInvoke.FreeLibrary(new HMODULE((void*)hModule));
        }
        // The other target frameworks bind this export by name ("vcmeta.dll"), which resolves to the vcmeta.dll module that
        // EnsureVcMetaIsLoaded loaded from its full path. Here, the export is looked up directly on that module instead.
        public static int HashForWinMD(string lpFileName, out Guid hash)
        {
            IntPtr address = VcMetaHandle == IntPtr.Zero ? IntPtr.Zero : GetProcAddress(VcMetaHandle, "HashForWinMD");
            if (address == IntPtr.Zero)
            {
                throw new EntryPointNotFoundException("Unable to find an entry point named 'HashForWinMD' in DLL 'vcmeta.dll'.");
            }

            hash = Guid.Empty;
            fixed (char* pFileName = lpFileName)
            fixed (Guid* pHash = &hash)
            {
                return ((delegate* unmanaged[Cdecl]<char*, Guid*, int>)address)(pFileName, pHash);
            }
        }
    }
}
