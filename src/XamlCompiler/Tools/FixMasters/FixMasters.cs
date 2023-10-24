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
            using (var contents = file.OpenText())
            {
                var encoding = contents.CurrentEncoding;
                var lines = new Queue<string>();
                while (!contents.EndOfStream)
                {
                    lines.Enqueue(FixLine(contents.ReadLine()));
                }
                contents.Close();
                Console.WriteLine(file.FullName);
                using (var writer = new StreamWriter(File.Open(file.FullName, FileMode.Truncate), Encoding.UTF8))
                {
                    while(lines.Count > 0)
                    {
                        writer.WriteLine(lines.Dequeue());
                    }
                }
            }
        }

        static string FixLine(string line)
        {
            string[] ignoreLines = {
                "#pragma checksum",
                "#ExternalChecksum",
                "// WARNING: Please don't edit this file",
            };

            string[] find = {
                "[global::System.CodeDom.Compiler.GeneratedCodeAttribute(\"Microsoft.UI.Xaml.Markup.Compiler\",\" 0.0.0.0\")]",
                "<Global.System.CodeDom.Compiler.GeneratedCodeAttribute(\"Microsoft.UI.Xaml.Markup.Compiler\", \" 0.0.0.0\")>  _",
            };

            string[] replace = {
                "[global::System.CodeDom.Compiler.GeneratedCodeAttribute(\"Microsoft.Windows.UI.Xaml.Build.Tasks\",\" 0.0.0.0\")]",
                "<Global.System.CodeDom.Compiler.GeneratedCodeAttribute(\"Microsoft.Windows.UI.Xaml.Build.Tasks\", \" 0.0.0.0\")>  _",
            };

            foreach (var ignoreLine in ignoreLines)
            {
                if (line.StartsWith(ignoreLine, StringComparison.OrdinalIgnoreCase))
                {
                    return $"{ignoreLine}...";
                }
            }

            for (int i = 0; i < find.Length; i++)
            {
                if (line.Contains(find[i]))
                {
                    return line.Replace(find[i], replace[i]);
                }
            }
            return line;
        }
    }
}
