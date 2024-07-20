// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public static class XamlMemberExtensions
    {
        internal static bool IsDependencyProperty(this XamlMember instance)
        {
            if (instance is IXamlMemberMeta meta)
            {
                return meta != null ? meta.IsDependencyProperty : false;
            }
            throw new ArgumentException("instance");
        }
    }
}
