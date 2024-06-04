// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public interface IBindUniverse
    {
        BindPathStep RootStep { get; }
        BindPathStep EnsureUniquePathStep(BindPathStep step);
        XamlType GetNamedElementType(string name, out string objectCodeName);
        XamlType GetNamedFieldType(string name);
        BindPathStep MakeOrGetRootStepOutOfScope();
    }
}
