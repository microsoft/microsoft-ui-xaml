// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Text;
using System.Runtime.InteropServices;
using System.Security;
using Microsoft.VisualStudio.ProjectSystem.Interop;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tasks
{
    // <summary>
    // Internal interface used for Interop in VS.NET hosting scenarios.
    // VS.NET project system prepares the hostobject instance which implements
    // this interface, The markupcompiler task wants to call this interface
    // to get file content from editor buffer and get last modification time.
    // </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("33372170-A08F-47F9-B1AE-CD9F2C3BB7C9")]
    internal interface IVsMSBuildTaskFileManager
    {
        // Returns the contents of the specified file based on whats in-memory else what's
        // on disk if not in-memory.
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        string GetFileContents([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename);

        // <summary>
        // Returns the live punkDocData object for the file if it is registered in the RDT,
        // else returns NULL.
        // </summary>
        ///<SecurityNote> 
        ///     Critical - call is SUC'ed
        ///</SecurityNote> 
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        [return: MarshalAs(UnmanagedType.IUnknown)]
        object GetFileDocData([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename);


        // Returns the time of the last change to the file. If open in memory, then this is the
        // time of the last edit as reported via IVsLastChangeTimeProvider::GetLastChangeTime
        // on the open document. If the file is not open, then the last change time of the file
        // on disk is returned.
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        //System.Runtime.InteropServices.ComTypes.FILETIME GetFileLastChangeTime([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename);
        long GetFileLastChangeTime([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename);

        // PutGeneratedFileContents -- puts the contents for the generated file
        // into an in memory TextBuffer and registers it in the RDT with a RDT_ReadLock.
        // This holds the file open in memory until the project is closed (when the
        // project will call IVsMSBuildHostObject::Close). If this is an actual
        // build operation (ie. UICONTEXT_SolutionBuilding is on) then the file will
        // also be saved to disk. If this is only a generation at design time for
        // intellisense purposes then the file contents are only put into memory
        // and the disk is not modified. The in-memory TextBuffer is always marked
        // as clean so the user will not be prompted to save the generated file.
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        void PutGeneratedFileContents([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename, [In, MarshalAs(UnmanagedType.LPWStr)] string strFileContents);


        // IsRealBuildOperation -- returns TRUE if this is a real Build operation else
        // if this is a design-time only generation for intellisense purposes it returns
        // FALSE.
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        [return: MarshalAs(UnmanagedType.Bool)]
        bool IsRealBuildOperation();


        // Delete -- deletes a file on disk and removes it from the RDT
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        void Delete([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename);


        // Exists -- determines whether or not a file exists in the RDT or on disk
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        [return: MarshalAs(UnmanagedType.Bool)]
        bool Exists([In, MarshalAs(UnmanagedType.LPWStr)] string wszFilename, [In, MarshalAs(UnmanagedType.Bool)] bool fOnlyCheckOnDisk);
    }


    [Serializable]
    internal class VSMSBuildTaskFileService : BuildTaskFileService
    {
        private VsMSBuildFileManagerHostObjectWrapper hostFileManager;

        public VSMSBuildTaskFileService(VsMSBuildFileManagerHostObjectWrapper taskFileManager, string langExtension)
            : base(langExtension)
        {
            hostFileManager = taskFileManager;
        }

        public override bool HasIdeHost
        {
            get { return hostFileManager != null; }
        }

        public override TextReader GetFileContents(string srcFile)
        {
            TextReader reader = null;
            if (hostFileManager != null)
            {
                reader = hostFileManager.GetFileContents(srcFile);
            }
            else
            {
                reader = base.GetFileContents(srcFile);
            }

            return reader;
        }

        public override DateTime GetLastChangeTime(string srcFile)
        {
            DateTime lastChangeDT;

            if (hostFileManager != null)
            {
                lastChangeDT = hostFileManager.GetFileLastChangeTime(srcFile);
            }
            else
            {
                lastChangeDT = base.GetLastChangeTime(srcFile);
            }

            return lastChangeDT;
        }

        public override void DeleteFile(string srcFile)
        {
            if (hostFileManager != null)
                hostFileManager.Delete(srcFile);
            else
            {
                base.DeleteFile(srcFile);
            };
        }

        public override bool IsRealBuild
        {
            get
            {
                if (hostFileManager != null)
                {
                    return hostFileManager.IsRealBuildOperation();
                }
                else
                {
                    return base.IsRealBuild;
                }
            }
        }
    }

    /// <summary>
    /// A wrapper that abstracts the differences between host objects implementing
    /// <see cref="IVsMSBuildTaskFileManager"/> and <see cref="IVsMSBuildTaskFreeThreadedFileManager"/>.
    /// </summary>
    internal class VsMSBuildFileManagerHostObjectWrapper : IVsMSBuildTaskFreeThreadedFileManager
    {
        /// <summary>
        /// The wrapped STA affinitized <see cref="IVsMSBuildTaskFileManager"/> host object.
        /// </summary>
        private IVsMSBuildTaskFileManager legacy;

        /// <summary>
        /// The wrapped free-threaded <see cref="IVsMSBuildTaskFreeThreadedFileManager"/> host object.
        /// </summary>
        private IVsMSBuildTaskFreeThreadedFileManager freeThreaded;

        /// <summary>
        /// Initializes a new instance of the <see cref="VsMSBuildFileManagerHostObjectWrapper"/> class
        /// that wraps the STA affinitized <see cref="IVsMSBuildTaskFileManager"/> host object.
        /// </summary>
        private VsMSBuildFileManagerHostObjectWrapper(IVsMSBuildTaskFileManager fileManager)
        {
            if (fileManager == null)
            {
                throw new ArgumentNullException();
            }

            this.legacy = fileManager;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VsMSBuildFileManagerHostObjectWrapper"/> class
        /// that wraps the free-threaded <see cref="IVsMSBuildTaskFreeThreadedFileManager"/> host object.
        /// </summary>
        private VsMSBuildFileManagerHostObjectWrapper(IVsMSBuildTaskFreeThreadedFileManager fileManager)
        {
            if (fileManager == null)
            {
                throw new ArgumentNullException();
            }

            this.freeThreaded = fileManager;
        }

        /// <summary>
        /// Checks whether we're running in a solution build (as opposed to a design-time build).
        /// </summary>
        public bool IsRealBuildOperation()
        {
            // Only the legacy host object is ever used in full builds.
            // The new host object is only used in design-time builds.
            return this.legacy != null && this.legacy.IsRealBuildOperation();
        }

        /// <summary>
        /// Marks the specified file for deletion.
        /// </summary>
        public void Delete(string fileName)
        {
            if (this.legacy != null)
            {
                this.legacy.Delete(fileName);
            }
            else
            {
                this.freeThreaded.Delete(fileName);
            }
        }

        /// <summary>
        /// Checks whether the specified file exists.
        /// </summary>
        public bool Exists(string fileName, bool onlyCheckOnDisk = false)
        {
            if (this.legacy != null)
            {
                return this.legacy.Exists(fileName, onlyCheckOnDisk);
            }
            else
            {
                return this.freeThreaded.Exists(fileName, onlyCheckOnDisk);
            }
        }

        /// <summary>
        /// Gets the contents of the specified file, from disk or unsaved buffers in the editor, if available.
        /// </summary>
        public TextReader GetFileContents(string fileName)
        {
            if (this.legacy != null)
            {
                // Use the host object to get the file contents - it  removes the BOM before 
                // returning the string.
                string strFileContent = this.legacy.GetFileContents(fileName);

                // For xaml file, UTF8 is the standard encoding so we need to convert it
                UTF8Encoding utf8Encoding = new UTF8Encoding();
                byte[] baFileContent = utf8Encoding.GetBytes(strFileContent);
                var stream = new MemoryStream(baFileContent);

                return new StreamReader(stream);
            }
            else
            {
                return this.freeThreaded.GetFileContents(fileName);
            }
        }

        /// <summary>
        /// Gets the last time this file's contents were changed (including in unsaved buffers).
        /// </summary>
        public DateTime GetFileLastChangeTime(string fileName)
        {
            if (this.legacy != null)
            {
                long fileTime = this.legacy.GetFileLastChangeTime(fileName);
                return DateTime.FromFileTime(fileTime);
            }
            else
            {
                return this.freeThreaded.GetFileLastChangeTime(fileName);
            }
        }

        /// <summary>
        /// Updates the editor buffers and/or disk with new contents for the specified file.
        /// </summary>
        public void PutGeneratedFileContents(string fileName, string contents)
        {
            if (this.legacy != null)
            {
                this.legacy.PutGeneratedFileContents(fileName, contents);
            }
            else
            {
                this.freeThreaded.PutGeneratedFileContents(fileName, contents);
            }
        }

        /// <summary>
        /// Creates a wrapper around the specified host object, if the type of object is recognizeable.
        /// </summary>
        /// <param name="hostObject">The host object.</param>
        /// <returns>A wrapper instance, or <c>null</c> if the host object wasn't of a recognized type.</returns>
        internal static VsMSBuildFileManagerHostObjectWrapper Acquire(object hostObject)
        {
            var freeThreaded = hostObject as IVsMSBuildTaskFreeThreadedFileManager;
            if (freeThreaded != null)
            {
                return new VsMSBuildFileManagerHostObjectWrapper(freeThreaded);
            }

            var legacy = hostObject as IVsMSBuildTaskFileManager;
            if (legacy != null)
            {
                return new VsMSBuildFileManagerHostObjectWrapper(legacy);
            }

            return null;
        }
    }
}

