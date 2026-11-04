// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml.Hosting;

namespace WinUICsIslandsSampleApp;

public partial class App : Microsoft.UI.Xaml.Application
{
    public App()
    {
        InitialWindowsXamlManager = WindowsXamlManager.InitializeForCurrentThread();

        try
        {
            InitializeComponent();
        }
        catch
        {
            InitialWindowsXamlManager.Dispose();
            InitialWindowsXamlManager = null;
            throw;
        }
    }

    public WindowsXamlManager? InitialWindowsXamlManager { get; private set; }

    public void ReleaseWindowsXamlManager()
    {
        InitialWindowsXamlManager?.Dispose();
        InitialWindowsXamlManager = null;
    }
}
