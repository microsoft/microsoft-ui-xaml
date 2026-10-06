// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using TableViewSampleApp;

namespace TableViewSampleApp.Pages;

public sealed partial class HomePage : Page
{
    private MainWindow? _mainWindow;

    public HomePage()
    {
        InitializeComponent();
    }

    protected override void OnNavigatedTo(NavigationEventArgs e)
    {
        base.OnNavigatedTo(e);
        _mainWindow = e.Parameter as MainWindow;
    }

    private void OnNavigateButtonClick(object sender, RoutedEventArgs e)
    {
        if (sender is Button { Tag: string tag })
        {
            _mainWindow?.NavigateTo(tag);
        }
    }
}
