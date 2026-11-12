// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.ProjectSystem.Interop;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    /// <summary>
    /// The file manager host object Visual Studio gives to the tasks named CompileXaml in design-time builds (see the
    /// MSBuildHostObjects registration of the project systems), to read the XAML files open in the editor instead of the disk.
    /// </summary>
    /// <remarks>
    /// This is the STA-affinitized interface, which is what the out-of-process host object of the CPS project system (SDK-style
    /// C# and VB projects) and the older project systems implement.
    /// </remarks>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("33372170-A08F-47F9-B1AE-CD9F2C3BB7C9")]
    internal interface IVsMSBuildTaskFileManager
    {
        // Returns the contents of the specified file based on what's in memory, else what's on disk if not in memory
        string GetFileContents([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename);

        // Returns the live punkDocData object for the file if it is registered in the RDT, else returns null
        [return: MarshalAs(UnmanagedType.IUnknown)]
        object GetFileDocData([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename);

        // Returns the time of the last change to the file (a FILETIME). If open in memory, then this is the time of the last
        // edit as reported via IVsLastChangeTimeProvider::GetLastChangeTime on the open document; if the file is not open, then
        // the last change time of the file on disk is returned.
        long GetFileLastChangeTime([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename);

        // Puts the contents for the generated file into an in-memory text buffer (not used)
        void PutGeneratedFileContents([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename, [In, MarshalAs(UnmanagedType.LPWStr)] string strFileContents);

        // Returns true if this is a real build operation, false if this is a design-time only generation for IntelliSense
        [return: MarshalAs(UnmanagedType.Bool)]
        bool IsRealBuildOperation();

        // Deletes a file on disk and removes it from the RDT (not used)
        void Delete([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename);

        // Determines whether or not a file exists in the RDT or on disk (not used)
        [return: MarshalAs(UnmanagedType.Bool)]
        bool Exists([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename, [In, MarshalAs(UnmanagedType.Bool)] bool fOnlyCheckOnDisk);
    }

    /// <summary>
    /// Reads the XAML files open in the IDE through its host object, either <see cref="IVsMSBuildTaskFileManager"/> or the
    /// free-threaded <see cref="IVsMSBuildTaskFreeThreadedFileManager"/> of the CPS project system (in-process design-time builds),
    /// which is embedded from the Microsoft.VisualStudio.ProjectSystem package.
    /// </summary>
    internal sealed class IdeHostFileManager
    {
        private readonly IVsMSBuildTaskFileManager _legacy;
        private readonly IVsMSBuildTaskFreeThreadedFileManager _freeThreaded;

        private IdeHostFileManager(IVsMSBuildTaskFileManager legacy, IVsMSBuildTaskFreeThreadedFileManager freeThreaded)
        {
            _legacy = legacy;
            _freeThreaded = freeThreaded;
        }

        /// <summary>Gets a file manager for the given host object, or null if it isn't one.</summary>
        public static IdeHostFileManager Acquire(object hostObject)
        {
            if (hostObject is IVsMSBuildTaskFreeThreadedFileManager freeThreaded)
            {
                return new IdeHostFileManager(null, freeThreaded);
            }
            if (hostObject is IVsMSBuildTaskFileManager legacy)
            {
                return new IdeHostFileManager(legacy, null);
            }
            return null;
        }

        public string Kind => _legacy != null ? nameof(IVsMSBuildTaskFileManager) : nameof(IVsMSBuildTaskFreeThreadedFileManager);

        /// <summary>
        /// Whether this is a real build rather than a design-time build. The free-threaded host object is only given to design-time
        /// builds, and doesn't say.
        /// </summary>
        public bool IsRealBuildOperation()
        {
            return _legacy != null && _legacy.IsRealBuildOperation();
        }

        /// <summary>Gets the last change time of a file (a FILETIME): the time of the last edit if it is open in the editor.</summary>
        public long GetFileLastChangeTime(string path)
        {
            return _legacy != null ? _legacy.GetFileLastChangeTime(path) : _freeThreaded.GetFileLastChangeTime(path).ToFileTime();
        }

        /// <summary>Gets the text of a file, from the editor if it is open (unsaved edits included), without its BOM.</summary>
        public string GetFileContents(string path)
        {
            if (_legacy != null)
            {
                return _legacy.GetFileContents(path);
            }
            using (TextReader reader = _freeThreaded.GetFileContents(path))
            {
                return reader.ReadToEnd();
            }
        }
    }
}
