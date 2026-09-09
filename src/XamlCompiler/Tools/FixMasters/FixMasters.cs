// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FixMasters
{
    class Program
    {
        static DirectoryInfo mastersPath = new DirectoryInfo(@".\TestMasters");

        static void Main(string[] args)
        {
            FixMasters(mastersPath);
        }

        static void FixMasters(DirectoryInfo dir)
        {
            foreach(var file in dir.EnumerateFiles())
            {
                FixMasters(file);
            }
            foreach(var subDir in dir.EnumerateDirectories())
            {
                FixMasters(subDir);
            }
        }

        static void FixMasters(FileInfo file)
        {
            // Whether the file has a byte order mark is a property of the generated file, not
            // something this tool gets to decide. Rewriting every master with one would churn the
            // masters of targets that were not regenerated, and would stop a second run of
            // copynewmasters.cmd from being a no-op.
            bool hasByteOrderMark = HasUtf8ByteOrderMark(file);

            var lines = new Queue<string>();
            using (var contents = file.OpenText())
            {
                while (!contents.EndOfStream)
                {
                    lines.Enqueue(FixLine(contents.ReadLine()));
                }
            }

            Console.WriteLine(file.FullName);
            using (var writer = new StreamWriter(File.Open(file.FullName, FileMode.Truncate), new UTF8Encoding(hasByteOrderMark)))
            {
                while (lines.Count > 0)
                {
                    writer.WriteLine(lines.Dequeue());
                }
            }
        }

        static bool HasUtf8ByteOrderMark(FileInfo file)
        {
            byte[] preamble = Encoding.UTF8.GetPreamble();
            byte[] start = new byte[preamble.Length];

            using (var stream = file.OpenRead())
            {
                if (stream.Read(start, 0, start.Length) < start.Length)
                {
                    return false;
                }
            }

            for (int i = 0; i < preamble.Length; i++)
            {
                if (start[i] != preamble[i])
                {
                    return false;
                }
            }

            return true;
        }

        static string FixLine(string line)
        {
            // Lines that are truncated because they carry a checksum or a tool version that
            // legitimately changes between builds. CodegenTests.IsException skips the same set when
            // diffing, so the two must be kept in agreement.
            string[] ignoreLines = {
                "#pragma checksum",
                "#ExternalChecksum",
                "// WARNING: Please don't edit this file",
            };

            foreach (var ignoreLine in ignoreLines)
            {
                if (line.StartsWith(ignoreLine, StringComparison.OrdinalIgnoreCase))
                {
                    return $"{ignoreLine}...";
                }
            }

            return line;
        }
    }
}
