// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    using System.Xaml;

    internal interface IDirectUIXamlLanguage
    {
        XamlType BindExtension { get; }
        XamlType Boolean { get; }
        XamlType CustomResourceExtension { get; }
        XamlType Double { get; }
        XamlType Int32 { get; }
        XamlType NullExtension { get; }
        XamlType Object { get; }
        XamlType StaticResourceExtension { get; }
        XamlType String { get; }
        XamlType UIElement { get; }
        XamlType Properties { get; }
        XamlType Property { get; }
        bool IsStringNullable { get; }

        XamlType LookupXamlObjects(string name);
    }
}