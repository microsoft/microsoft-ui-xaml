// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    internal interface IXamlDomNode
    {
        string SourceFilePath { get; }
        int EndLineNumber { get; set; }
        int EndLinePosition { get; set; }
        bool IsSealed { get; }
        int StartLineNumber { get; set; }
        int StartLinePosition { get; set; }

        void Seal();
    }
}