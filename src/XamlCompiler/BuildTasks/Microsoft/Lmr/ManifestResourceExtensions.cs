// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using System.IO;
using System.Reflection;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    internal static class ManifestResourceExtensions
    {
        // Gets the contents of a resource that is embedded in the assembly
        public static byte[] GetManifestResourceBytes(this Assembly assembly, string resourceName)
        {
            using (Stream stream = assembly.GetManifestResourceStream(resourceName))
            {
                if (stream == null)
                {
                    throw new FileNotFoundException(string.Format(CultureInfo.InvariantCulture,
                        "The embedded resource '{0}' was not found in '{1}'.", resourceName, assembly.GetName().Name));
                }

                using (MemoryStream memory = new MemoryStream())
                {
                    stream.CopyTo(memory);
                    return memory.ToArray();
                }
            }
        }
    }
}
