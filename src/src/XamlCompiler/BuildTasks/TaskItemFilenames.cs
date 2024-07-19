// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.IO;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using MSBuildInterop;
    using Utilities;

    internal class TaskItemFilename
    {
        public string SourceXamlFullPath { get; private set; }
        public string XamlGivenPath { get; private set; }
        public string TargetFolder { get; private set; }
        public string FileNameNoExtension { get; private set; }
        public string XamlOutputFilename { get; private set; }
        public string XbfOutputFilename { get; private set; }
        public string RelativePathFromGeneratedCodeToXamlFile { get; private set; }
        public string ApparentRelativePath { get; private set; }
        public DateTime XamlOutputChangeTime { get; private set; }
        public DateTime XbfOutputChangeTime { get; private set; }

        // This is in Ticks because it is serialized, and this is an easier format.
        public long XamlFileTimeAtLastCompile { get; set; }
        public DateTime XamlLastChangeTime { get; set; }
        public string GeneratedCodePathPrefix { get; set; }
        public bool IsApplication { get; private set; }
        public bool IsSdkXamlFile { get; private set; }
        public string TargetPathMetadata { get; set; }
        public string ClassFullName { get; set; }
        public bool IsForcedOutOfDate { get; set; }

        public string XamlResourceMapName { get; private set; }
        public string XamlComponentResourceLocation { get; private set; }

        private bool _outputFileIsZeroLength;
        private bool _xbfFileIsZeroLength;
        private SourceFileManager _srcMgr;

        public TaskItemFilename(IFileItem item, SourceFileManager srcMgr, bool isApplication, bool isSdkXaml)
        {
            this.IsApplication = isApplication;
            this.IsSdkXamlFile = isSdkXaml;
            this._srcMgr = srcMgr;

            XamlResourceMapName = item.MSBuild_XamlResourceMapName;
            XamlComponentResourceLocation = item.MSBuild_XamlComponentResourceLocation;

            // If there is a "Link" attribute use that, otherwise use the ItemSpec
            // The Link property is where the file "appears" to live.
            // ItemSpec if where it really sits.
            // The "apparent" path will be used to compute the output path.
            string linkPath = item.MSBuild_Link;
            if (String.IsNullOrEmpty(linkPath))
            {
                string fileFullpath = item.FullPath;
                // GetDefaultXamlLinkMetadata() will return null if the item.ItemSpec
                // is a normal usable relative path inside the project tree.
                linkPath = CompileXamlInternal.GetDefaultXamlLinkMetadata(fileFullpath, item.ItemSpec, _srcMgr.ProjectFolderFullpath, _srcMgr.IncludeFolderList);
            }

            // The Link property is not allowed to rooted or ".." up.
            // if the Link looks suspicious dump it for ItemSpec.
            if (string.IsNullOrEmpty(linkPath) || Path.IsPathRooted(linkPath) || linkPath.Contains(@"..\"))
            {
                // If the ItemSpec points outside the project the "apparent" path will be "." (the project dir)
                if (Path.IsPathRooted(item.ItemSpec) || item.ItemSpec.Contains(@"..\"))
                {
                    this.ApparentRelativePath = Path.GetFileName(item.ItemSpec);
                }
                else
                {
                    this.ApparentRelativePath = item.ItemSpec;
                }
            }
            else
            {
                this.ApparentRelativePath = linkPath;
            }

            this.TargetPathMetadata = item.MSBuild_TargetPath;
            if (isSdkXaml)
                Debug.Assert(!String.IsNullOrWhiteSpace(TargetPathMetadata), "SDK XAML file w/o a TargetPath");

            this.XamlGivenPath = item.ItemSpec;
            this.SourceXamlFullPath = item.ItemSpec;

            if (!Path.IsPathRooted(SourceXamlFullPath))
            {
                this.SourceXamlFullPath = Path.Combine(srcMgr.ProjectFolderFullpath, item.ItemSpec);
            }

            // normalize the path (if necessary)
            this.SourceXamlFullPath = Path.GetFullPath(SourceXamlFullPath);

            // When generating output files put SDK generated XBF files into sub-folders under the Target folder.
            string outputRelativePath = isSdkXaml ? this.TargetPathMetadata : this.ApparentRelativePath;
            this.TargetFolder = Path.Combine(srcMgr.OutputFolderFullpath, Path.GetDirectoryName(outputRelativePath));

            // Take Foo.xaml filename and generate a Foo.g.cs or Foo.g.i.cs.   GeneratedExtension is "pass" dependent.
            // VB filenames are Foo.g.vb and Foo.g.i.vb.   C++ filenames are Foo.g.hpp and Foo.g.h
            // The C++ pass2 extension is hpp (rather than cpp) because to improve the speed of project compile time
            // they are #included into the XamlTypeInfo.cpp
            this.FileNameNoExtension = Path.GetFileNameWithoutExtension(SourceXamlFullPath);
            this.XamlOutputFilename = Path.Combine(TargetFolder, FileNameNoExtension + KnownStrings.XamlExtension);
            this.XbfOutputFilename = Path.Combine(TargetFolder, FileNameNoExtension + KnownStrings.XbfExtension);
            SaveStatePerXamlFile saveXamlFileState;
            if (_srcMgr.SavedState.XamlPerFileInfo.TryGetValue(this.XamlGivenPath, out saveXamlFileState))
            {
                this.GeneratedCodePathPrefix = saveXamlFileState.GeneratedCodeFilePathPrefix;
            }
            this.RelativePathFromGeneratedCodeToXamlFile = FileHelpers.GetRelativePath(Path.GetDirectoryName(TargetFolder), this.ApparentRelativePath);

            Refresh(_srcMgr.SavedState);
        }

        public void Refresh(SavedStateManager saveState)
        {
            // If the file does not exist the time returned is Jan 1 1601.
            this.XamlOutputChangeTime = File.GetLastWriteTime(XamlOutputFilename);
            if (!_srcMgr.XbfGenerationIsDisabled)
            {
                this.XbfOutputChangeTime = File.GetLastWriteTime(XbfOutputFilename);
            }
            XamlFileTimeAtLastCompile = saveState.GetXamlFileTimeAtLastCompile(XamlGivenPath);

            _outputFileIsZeroLength = true;

            // Default is <Xaml_File_Name>.g.(i.)cs : To handle Resource Dictionaries that have no classes, and thus have empty GeneratedCodePathPrefixes
            string generatedFileFullPath = Path.Combine(TargetFolder, FileNameNoExtension + this._srcMgr.GeneratedFileExtension);
            if (!String.IsNullOrEmpty(this.GeneratedCodePathPrefix))
            {
                generatedFileFullPath = this.GeneratedCodePathPrefix + this._srcMgr.GeneratedFileExtension;
            }

            if (File.Exists(generatedFileFullPath))
            {
                FileInfo fi = new FileInfo(generatedFileFullPath);
                _outputFileIsZeroLength = (fi.Length == 0);
            }

            _xbfFileIsZeroLength = true;
            if (File.Exists(this.XbfOutputFilename))
            {
                FileInfo xfi = new FileInfo(this.XbfOutputFilename);
                _xbfFileIsZeroLength = (xfi.Length == 0);
            }

            this.IsForcedOutOfDate = false;
        }

        public bool OutOfDate()
        {
            // Note #1) After a design time build generates the g.i file, from a dirty IDE buffer, the user
            // can abandon source changes in the IDE and xaml file last change date will revert to the
            // original file time.  This gives the apperence of the file moving backward in time.
            // In this case we still need to rebuild.
            // A rebuild triggered by the above will also regnerated the XBF (if not disabled),
            // so simple "later than" tests are approprate for XBF files.
            // Note #2) A missing file has a very old date (Jan 1 1601)

            ////
            // All the reasons a XAML file could be "out of date"

            // The Output Source file is zero lengh.
            //  When the XAML is dirty, Pass1 will zero out the Pass2 class code
            //  file to ensure a clean build.  When Pass2 comes along the zero
            //  length Pass2 file will have a later date than the XAML, but we still
            //  need to build it.  Thus the special case.
            if (_outputFileIsZeroLength)
            {
                return true;
            }

            if (this.IsForcedOutOfDate)
            {
                return true;
            }

            if (_srcMgr.IsPass1)
            {
                // We can't just compare a.xaml vs. a.g.i.cs because to the "abandoned IDE buffer"
                // issue described in Note #1 above.
                if (XamlLastChangeTime.Ticks != XamlFileTimeAtLastCompile)
                {
                    return true;
                }
            }
            else
            {
                // In Pass2

                // Comparing against generated *code* files is problematic because
                // XAML files w/o x:Class do not have generated code.
                // But... all XAML files have edited XAML (for packaging).
                // The Source XAML is later than the Edited XAML
                if (XamlLastChangeTime > XamlOutputChangeTime)
                {
                    return true;
                }

                // if we are generating XBF
                if (!_srcMgr.XbfGenerationIsDisabled)
                {
                    // The XBF is zero length or Older than the Source XAML
                    // Zero length XBF file implies a build failure.
                    if (_xbfFileIsZeroLength || XamlLastChangeTime > XbfOutputChangeTime)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

    }
}

