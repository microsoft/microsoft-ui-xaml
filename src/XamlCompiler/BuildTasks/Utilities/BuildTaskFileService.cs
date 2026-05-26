// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    // Abstracts away the file system and IVsMSBuildTaskFileManager to callers. This is used by
    // the markupcompiler, but any build task can use it.
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

            using (StreamWriter sw = new StreamWriter(destinationFile, false, new UTF8Encoding(true)))
            {
                sw.WriteLine(fileContents);
            }
        }

        public virtual bool IsRealBuild
        {
            get { return true; }
        }
    }
}

