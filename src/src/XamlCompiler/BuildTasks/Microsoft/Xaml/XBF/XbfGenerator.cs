// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XBF
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Runtime.InteropServices.ComTypes;
    using FileIO;
    using Utilities;
    
    internal enum XbfErrorConstants : int
    {
        PropertyNotFound = 0x09c4,
    }

    internal class XbfGenerator
    {
        private static Dictionary<Version, IntPtr> s_GenXbfDllHandles = new Core.InstanceCache<Version, IntPtr>(ClearCache);
        private List<IXbfFileNameInfo> xamlFiles = new List<IXbfFileNameInfo>();
        private IXbfMetadataProvider xbfMetadataProvider;
        private XamlProjectInfo projectInfo;

        private Lazy<List<XamlCompileWarning>> warningMessages = new Lazy<List<XamlCompileWarning>>(() => new List<XamlCompileWarning>());
        private Lazy<List<XamlCompileError>> errorMessages = new Lazy<List<XamlCompileError>>(() => new List<XamlCompileError>());

        public List<XamlCompileWarning> XbfWarnings { get { return warningMessages.Value; } }
        public List<XamlCompileError> XbfErrors { get { return errorMessages.Value; } }

        public XbfGenerator(XamlProjectInfo projectInfo, IXbfMetadataProvider xbfMetadataProvider)
        {
            this.projectInfo = projectInfo;
            this.xbfMetadataProvider = xbfMetadataProvider;
        }

        public void SetXamlInputFilesFromTaskItems(IEnumerable<TaskItemFilename> tifs, bool isSdk = false)
        {
            foreach (var tif in tifs)
            {
                this.xamlFiles.Add(new XbfFileNameInfo(tif.SourceXamlFullPath, tif.XamlGivenPath, isSdk ? tif.XamlGivenPath : tif.XamlOutputFilename, tif.XbfOutputFilename));
            }
        }

        public void SetXamlInputFiles(IEnumerable<IXbfFileNameInfo> xamlFiles)
        {
            this.xamlFiles.AddRange(xamlFiles);
        }

        public bool GenerateXbfFiles(uint xbfGenerationFlags, bool v80Compat = false)
        {
            var dllHandle = IntPtr.Zero;
            var dllPath = string.Empty;
            switch (RuntimeInformation.ProcessArchitecture)
            {
                case Architecture.Arm64:
                    dllPath = this.projectInfo.GenXbfArm64Path;
                    break;
                case Architecture.X64:
                    dllPath = this.projectInfo.GenXbf64Path;
                    break;
                case Architecture.X86:
                default:
                    dllPath = this.projectInfo.GenXbf32Path;
                    break;
            }

            // No path was given to the Xaml Compiler.
            if (string.IsNullOrWhiteSpace(this.projectInfo.GenXbf32Path))
            {
                this.XbfErrors.Add(new XbfGeneration_NoWindowsSdk());
                return false;
            }

            // Try to load the dll, unless already loaded.
            if (!s_GenXbfDllHandles.TryGetValue(this.projectInfo.TargetPlatformMinVersion, out dllHandle))
            {
                dllHandle = NativeMethods.LoadLibraryEx(dllPath, IntPtr.Zero, NativeMethods.LOAD_WITH_ALTERED_SEARCH_PATH);
                s_GenXbfDllHandles.Add(this.projectInfo.TargetPlatformMinVersion, dllHandle);
            }

            // Ensure we have a valid handle.
            if (dllHandle == IntPtr.Zero)
            {
                this.XbfErrors.Add(new XbfGeneration_CouldNotLoadXbfGenerator(dllPath));
                return false;
            }

            // Validate files have the right extension.
            foreach (var tif in this.xamlFiles)
            {
                if (!tif.GivenXamlName.EndsWith(KnownStrings.XamlExtension, StringComparison.OrdinalIgnoreCase))
                {
                    this.XbfErrors.Add(new XamlFileMustEndInDotXaml(tif.GivenXamlName));
                    return false;
                }
            }

            // For v8.0 compat turn off SetAirityOnGenericTypeNames
            bool savedMode = XamlSchemaCodeInfo.SetAirityOnGenericTypeNames;

            XamlSchemaCodeInfo.SetAirityOnGenericTypeNames = !v80Compat;
            bool ret = this.GenerateAll(dllHandle, xbfGenerationFlags);
            XamlSchemaCodeInfo.SetAirityOnGenericTypeNames = savedMode;
            return ret;
        }

        internal virtual List<IStream> GetInputOutputStreams()
        {
            List<IStream> streams = new List<IStream>();

            // Build the list of in/out Streams.
            foreach (XbfFileNameInfo xamlFile in this.xamlFiles)
            {
                string xamlFileName = xamlFile.InputXamlName;
                try
                {
                    streams.Add(new StreamXamlInput(xamlFileName));
                }
                catch (Exception ex)  // permission denied, UNC path problem, etc...
                {
                    this.XbfErrors.Add(new XbfInputFileOpenFailure(xamlFileName, ex.Message));
                    return null;
                }

                string xbfFileName = xamlFile.OutputXbfName;
                try
                {
                    string folder = Path.GetDirectoryName(xbfFileName);
                    Directory.CreateDirectory(folder);

                    streams.Add(new StreamXbfOutput(xbfFileName));
                }
                catch (Exception ex)  // permission denied, UNC path problem, etc...
                {
                    this.XbfErrors.Add(new XbfOutputFileOpenFailure(xbfFileName, ex.Message));
                    return null;
                }
            }

            return streams;
        }

        internal virtual bool GenerateXbfFromStreams(
            IntPtr dllHandle,
            IStream[] inputStreams,
            IStream[] outputStreams,
            uint xbfGenerationFlags,
            string[] checksums,
            TargetOSVersion targetOS,
            out int errorCode,
            out int errorFile,
            out int errorLine,
            out int errorPosition)
        {
            errorCode = errorFile = errorLine = errorPosition = 0;
            try
            {

                // this shouldn't throw because they should report what went wrong.
                int result = NativeMethodsHelper.Write(
                    dllHandle,
                    inputStreams,
                    inputStreams.Count(),
                    checksums,
                    ChecksumHelper.ChecksumLength,
                    this.xbfMetadataProvider,
                    targetOS,
                    xbfGenerationFlags,
                    outputStreams,
                    out errorCode,
                    out errorFile,
                    out errorLine,
                    out errorPosition);
            }
            catch (Exception ex)
            {
                this.XbfErrors.Add(new XbfGenerationGeneralFailure(ex.Message));
                return false;
            }

            return true;
        }

        internal virtual string GetTextLine(string fileName, int line)
        {
            using (TextReader textReader = File.OpenText(fileName))
            {
                string textLine = string.Empty;
                for (int i = 0; i < line; i++)
                {
                    textLine = textReader.ReadLine();
                }

                return textLine;
            }
        }

        private string[] GetAllXamlFilesChecksums()
        {
            IList<string> result = new List<string>();
            foreach (var file in this.xamlFiles)
            {
                result.Add(file.XamlFileChecksum);
            }
            return result.ToArray();
        }


        private bool GenerateAll(IntPtr dllHandle, uint xbfGenerationFlags)
        {
            int errorCode;
            int errorFile;
            int errorLine;
            int errorPosition;

            List<IStream> streams = this.GetInputOutputStreams();

            if (streams == null || streams.Count == 0)
            {
                return false;
            }

            try
            {
                IEnumerable<IStream> inputStreamsArray = streams.Where(s => (s is IXamlStream) && ((IXamlStream)s).StreamType == StreamType.Input);
                IEnumerable<IStream> outputStreamsArray = streams.Where(s => (s is IXamlStream) && ((IXamlStream)s).StreamType == StreamType.Output);

                if (!this.GenerateXbfFromStreams(
                    dllHandle,
                    inputStreamsArray.ToArray(),
                    outputStreamsArray.ToArray(),
                    xbfGenerationFlags,
                    this.GetAllXamlFilesChecksums(),
                    this.projectInfo.TargetPlatformMinVersion.ToTargetOSVersion(),
                    out errorCode,
                    out errorFile,
                    out errorLine,
                    out errorPosition))
                {
                    return false;
                }

                if (errorCode != 0)
                {
                    this.LogXbfError(errorCode, errorFile, errorLine, errorPosition);
                    return false;
                }
            }
            finally
            {
                foreach (StreamImpl stream in streams.OfType<StreamImpl>())
                {
                    stream.Dispose();
                }
            }

            return true;
        }

        private void LogXbfError(int errorCode, int errorFile, int errorLine, int errorPosition)
        {
            string fileName = this.xamlFiles[errorFile].GivenXamlName;
            string additional;

            if (this.IsMarkupExtensionAssignmentError(fileName, errorLine, errorPosition, out additional))
            {
                this.XbfErrors.Add(new XbfGeneration_NonMeInCurlyBraces(fileName, errorLine, errorPosition, additional, errorCode));
            }
            else  if (errorCode == (int)XbfErrorConstants.PropertyNotFound)
            {
                this.XbfErrors.Add(new XbfGenerationPropertyNotFoundError(fileName, errorLine, errorPosition));
            }
            else
            {
                this.XbfErrors.Add(new XbfGenerationParseError(fileName, errorLine, errorPosition, errorCode));
            }
        }

        private bool IsMarkupExtensionAssignmentError(string fileName, int line, int pos, out string additional)
        {
            string markupExtensionName;
            string textLine = this.GetTextLine(fileName, line);

            if (this.LookslikeMarkupExtensionAssigment(textLine, pos, out markupExtensionName))
            {
                additional = markupExtensionName;
                return true;
            }

            additional = string.Empty;
            return false;
        }

        // Although System.Xaml thinks it is just fine to put non-ME's in {}'s
        // The Jupiter runtime (and WPF compiler) think it is an error.
        // So we need to report it as an error here as well.
        private bool LookslikeMarkupExtensionAssigment(string textLine, int pos, out string meName)
        {
            if (pos < textLine.Length)
            {
                // compute the first index to either a ' or a "
                int idx1 = textLine.IndexOf('\'', pos);
                int idx2 = textLine.IndexOf('\"', pos);
                int idx = (idx1 == -1) ? idx2 : (idx2 == -1) ? idx1 : (idx1 < idx2) ? idx1 : idx2;

                if (idx != -1 && idx < textLine.Length - 3)
                {
                    if (textLine[idx + 1] == '{')
                    {
                        meName = ScanIdentifierToken(textLine, idx + 2);
                        if (!string.IsNullOrWhiteSpace(meName))
                        {
                            return true;
                        }
                    }
                }
            }
            else
            {
                // Pos is not a character index within the line string. It is the column where the error happened.
                // Pos can be the end of the line, which for a 63 character line is 64 (that's valid).
                // We will assert if this value exceeds the allowed.
                Debug.Assert(
                    pos <= textLine.Length + 1,
                    String.Format("GenXBF.dll is returning an invalid line position '{0}' for markup '{1}' which is '{2}' characters in length", pos, textLine, textLine.Length));
            }
            meName = string.Empty;
            return false;
        }

        private string ScanIdentifierToken(string textLine, int start)
        {
            string nameLine = textLine.Substring(start);

            int firstBad;
            XamlDomValidator.IsValidIdentifierName(nameLine, out firstBad);

            string name = nameLine.Substring(0, firstBad);
            return name;
        }

        private static void ClearCache()
        {
            foreach (var handle in s_GenXbfDllHandles.Values)
            {
                NativeMethods.FreeLibrary(handle);
            }
            s_GenXbfDllHandles.Clear();
        }
    }
}