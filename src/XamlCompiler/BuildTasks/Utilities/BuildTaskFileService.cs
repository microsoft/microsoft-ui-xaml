// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    // Abstracts away the file system to the compiler: in design-time builds, the files open in the IDE are read from the
    // overlays the CompileXaml task gets from Visual Studio instead (see OverlayTaskFileService).
    internal class BuildTaskFileService
    {
        private string langExtension;

        public BuildTaskFileService(string languageExtension)
        {
            this.langExtension = languageExtension;
        }

        public virtual bool HasIdeHost
        {
            get { return false; }
        }

        public virtual TextReader GetFileContents(string srcFile)
        {
            return new StreamReader(File.OpenRead(srcFile));
        }

        public virtual DateTime GetLastChangeTime(string srcFile)
        {
            return File.GetLastWriteTime(srcFile); ;
        }

        public bool FileExists(string srcFile)
        {
            return File.Exists(srcFile);
        }

        public virtual void DeleteFile(string srcFile)
        {
            if (File.Exists(srcFile))
            {
                File.Delete(srcFile);
            }
        }

        public void WriteFile(string fileContents, string destinationFile)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(destinationFile));

            AtomicFile.Write(destinationFile, stream =>
            {
                using (StreamWriter sw = new StreamWriter(stream, new UTF8Encoding(true)))
                {
                    sw.WriteLine(fileContents);
                }
            });
        }

        public virtual bool IsRealBuild
        {
            get { return true; }
        }
    }
}

