// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//
// SPLIT-BINARY consumption shim: in the split layout TableView and its family live in
// Microsoft.UI.Xaml.Controls.Tabular.dll, whose XAML metadata is exposed by
// Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsTabularXamlMetaDataProvider. Initialize() wires
// its activation so the app metadata provider chain can resolve the Tabular template types.

namespace TableViewSampleApp;

internal static class TabularMetadataProviderLoader
{
    /// <summary>
    /// Initializes the Tabular XAML metadata provider from the split control DLL and returns the
    /// provider instance so the caller can insert it into the app metadata provider's
    /// OtherProviders chain. This is what lets the runtime XamlReader.Load resolve the split-only
    /// Tabular template types (e.g. adv:TabularControlsResources).
    /// </summary>
    public static Microsoft.UI.Xaml.Markup.IXamlMetadataProvider? Create()
    {
        Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsTabularXamlMetaDataProvider.Initialize();
        return new Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsTabularXamlMetaDataProvider();
    }
}
