// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using TableViewSampleApp.Services;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Single source of truth for the persisted app theme. Reads
/// <see cref="AppSettings.LoadTheme"/> on navigation in, writes via
/// <see cref="AppSettings.ApplyAndPersist"/> on selection change.
/// The shell title-bar theme button reuses the same helper so both
/// surfaces stay in lockstep with no observer plumbing here.
///
/// Settings layout: a ComboBox theme picker plus a Storage card,
/// styled to mirror the CommunityToolkit SettingsCard look using the
/// sample's own Border/Grid + FontIcon primitives (the toolkit package is
/// not referenced by this gallery, so no dependency is taken). The picker
/// is ThemeModeCombo (ComboBoxItem Tags drive the ElementTheme) and Storage
/// surfaces the on-disk theme.txt path verbatim.
/// </summary>
public sealed partial class SettingsPage : Page
{
    // Guards SelectionChanged so the programmatic pre-select during
    // Loaded doesn't re-fire ApplyAndPersist (which would no-op but
    // also raise ThemeChanged needlessly).
    private bool _initializing;

    public SettingsPage()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        // -------- Theme picker: select the ComboBoxItem whose Tag matches
        // the persisted ElementTheme. ComboBox doesn't auto-select by Tag,
        // so we walk Items once at load time.
        _initializing = true;
        try
        {
            var current = AppSettings.LoadTheme();
            var targetTag = current switch
            {
                ElementTheme.Light => "Light",
                ElementTheme.Dark => "Dark",
                _ => "Default",
            };

            foreach (var item in ThemeModeCombo.Items)
            {
                if (item is ComboBoxItem cbi && (cbi.Tag as string) == targetTag)
                {
                    ThemeModeCombo.SelectedItem = cbi;
                    break;
                }
            }
        }
        finally
        {
            _initializing = false;
        }

        // -------- Storage card: show the user where the setting lives.
        // Useful when troubleshooting or copying settings between dev
        // boxes. Unchanged from the pre-Q4 page.
        var path = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "TableViewSampleApp",
            "theme.txt");
        StoragePathText.Text = path;
    }

    private void OnThemeSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_initializing) return;
        if (sender is not ComboBox combo || combo.SelectedItem is not ComboBoxItem picked) return;

        var theme = (picked.Tag as string) switch
        {
            "Light" => ElementTheme.Light,
            "Dark" => ElementTheme.Dark,
            _ => ElementTheme.Default,
        };

        var root = (App.Current as App)?.MainWindowContent as FrameworkElement;
        AppSettings.ApplyAndPersist(root, theme);
    }
}
