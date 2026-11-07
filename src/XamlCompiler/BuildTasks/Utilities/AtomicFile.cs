// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Threading;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    /// <summary>
    /// Writes files through a temporary file in the same folder, which then replaces the destination, so that terminating the
    /// compiler in the middle of a write (e.g. when MSBuild cancels a build and kills the process) can't leave a truncated
    /// output behind that a later incremental build would consider up to date.
    /// </summary>
    internal static class AtomicFile
    {
        private const int MoveAttempts = 5;

        public static void Write(string path, Action<Stream> write)
        {
            string fullPath = Path.GetFullPath(path);
            string tempPath = fullPath + "." + Guid.NewGuid().ToString("N").Substring(0, 8) + ".tmp";
            try
            {
                using (FileStream stream = new FileStream(tempPath, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                {
                    write(stream);
                }
                MoveWithRetries(tempPath, fullPath);
            }
            finally
            {
                TryDelete(tempPath);
            }
        }

        // The destination can be locked for a moment by another process (an antivirus scanner, the indexer, an editor reading it)
        private static void MoveWithRetries(string source, string destination)
        {
            for (int attempt = 1; ; attempt++)
            {
                try
                {
#if NET
                    File.Move(source, destination, overwrite: true);
#else
                    if (File.Exists(destination))
                    {
                        File.Replace(source, destination, null, ignoreMetadataErrors: true);
                    }
                    else
                    {
                        File.Move(source, destination);
                    }
#endif
                    return;
                }
                catch (Exception e) when ((e is IOException || e is UnauthorizedAccessException) && attempt < MoveAttempts)
                {
                    Thread.Sleep(50 * attempt);
                }
            }
        }

        // Must not hide the exception of the write itself
        private static void TryDelete(string path)
        {
            try
            {
                if (File.Exists(path))
                {
                    File.Delete(path);
                }
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
            {
            }
        }

        public static void WriteAllBytes(string path, byte[] bytes)
        {
            Write(path, stream => stream.Write(bytes, 0, bytes.Length));
        }
    }
}
