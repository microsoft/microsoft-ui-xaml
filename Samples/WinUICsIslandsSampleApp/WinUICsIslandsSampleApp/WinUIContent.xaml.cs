// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml;

namespace WinUICsIslandsSampleApp;

public sealed partial class WinUIContent : Microsoft.UI.Xaml.Controls.UserControl
{
    public WinUIContent()
    {
        InitializeComponent();
    }

    public void SetGeneration(int generation)
    {
        GenerationText.Text = $"WinUI generation {generation}";
    }

    private void IncrementButton_Click(object sender, RoutedEventArgs args)
    {
        ValueNumberBox.Value++;
    }
}
