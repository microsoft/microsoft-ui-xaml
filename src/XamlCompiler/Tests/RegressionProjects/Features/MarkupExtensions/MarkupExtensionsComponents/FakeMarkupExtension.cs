// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;

namespace MarkupExtensionsComponents
{
    [Bindable]
    public sealed class FakeMarkupExtension : Microsoft.UI.Xaml.Markup.MarkupExtension
    {
        public FakeMarkupExtension(IXamlMetadataProvider provider)
        {
            var type = provider.GetXamlType("MarkupExtensionsComponents.FakeMarkupExtension");
            if (type == null)
            {
                throw new ArgumentException("MarkupExtensionsComponents.FakeMarkupExtension is missing");
            }
            if (!type.IsMarkupExtension)
            {
                throw new ArgumentException("MarkupExtensionsComponents.FakeMarkupExtension is not a markup extension");
            }
        }
    }

}
