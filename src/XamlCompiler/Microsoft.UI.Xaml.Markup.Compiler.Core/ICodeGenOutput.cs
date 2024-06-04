// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public delegate string CodeGenDelegate();

    public interface ICodeGenOutput
    {
        CodeGenDelegate CppCXName { get; }
        CodeGenDelegate CppWinRTName { get; }
        CodeGenDelegate CSharpName { get; }
        CodeGenDelegate VBName { get; }
    }
}
