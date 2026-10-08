// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;

namespace TableViewSampleApp.Converters;

/// <summary>The converter keys every page resolves with {StaticResource}; merged once in App.xaml.</summary>
public sealed partial class SampleConverters : ResourceDictionary
{
    public SampleConverters()
    {
        InitializeComponent();
    }
}