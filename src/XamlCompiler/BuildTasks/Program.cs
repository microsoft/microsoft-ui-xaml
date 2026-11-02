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
                // So that XAML files in a Windows code page (e.g. encoding="windows-1252") can be read like with .NET Framework
                System.Text.Encoding.RegisterProvider(System.Text.CodePagesEncodingProvider.Instance);
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
