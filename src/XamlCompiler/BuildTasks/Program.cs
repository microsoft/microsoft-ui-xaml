// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;

namespace Microsoft.UI.Xaml.Markup.Compiler.Executable
{
    internal class Program
    {
        internal static int Main(string[] args)
        {
            try
            {
                CompileXaml compiler = new CompileXaml();
                return compiler.Run(args);
            }
            catch (System.Exception e)
            {
                System.Console.Error.Write($"{Properties.XamlCompilerResources.XamlInternlError}: {e.Message}");
                return 1;
            }
        }
    };
}
