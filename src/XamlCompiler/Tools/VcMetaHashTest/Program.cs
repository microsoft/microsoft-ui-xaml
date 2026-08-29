// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace VcMetaHashTest
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Error("Usage: VcMetaHashText <filename>");
                return;
            }
            if (!LoadVcMeta())
            {
                Error("Cannot load VCMeta.dll");
                return;
            }
            foreach (String filename in args)
            {
                int hresult;
                Guid hash;

                try
                {
                    hresult = HashForWinMD(filename, out hash);
                }
                catch (Exception ex)
                {
                    Error(String.Format("Cannot Hash {0}.  {1}", filename, ex.Message));
                    continue;
                }
                Console.WriteLine("{0} - {1}", hash.ToString(), filename);
            }
        }

        static void Error(string message)
        {
            Console.Error.WriteLine("{0}", message);
        }

        static bool LoadVcMeta()
        {
            bool vcMetaIsLoaded = false;
            const string vcInstallDir = @"C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\";

            try
            {
                // Try vcmeta - if this throws, its beause we can't find it
                Guid currentHash;
                int hresult = HashForWinMD(String.Empty, out currentHash);
                vcMetaIsLoaded = true;
            }
            catch (Exception)
            {
                // We can fault if we are 64bit, and load the 32bit vcmeta, OR, vcmeta isn't on the path
                String currentPath = System.Environment.GetEnvironmentVariable("path");
                try
                {
                    // If we are 64bit, use 64bit meta
                    bool is64BitProcess = System.Environment.Is64BitProcess;
                    string binDirSuffix = is64BitProcess ? @"\amd64" : "";

                    // Set the path with the 64bit FIRST, try to resolve vcmeta again.
                    System.Environment.SetEnvironmentVariable("path", Path.Combine(vcInstallDir, "bin" + binDirSuffix) + ";" + currentPath);
                    Guid currentHash;
                    int hresult = HashForWinMD(String.Empty, out currentHash);
                    vcMetaIsLoaded = true;
                }
                catch (Exception)
                {
                    // An un-expected error
                }
                finally
                {
                    System.Environment.SetEnvironmentVariable("path", currentPath);
                }
            }
            return vcMetaIsLoaded;
        }

        [DllImport("vcmeta.dll", CharSet = CharSet.Auto, CallingConvention = CallingConvention.Cdecl)]
        static extern int HashForWinMD(String lpFileName, out Guid hash);
    }

}
