// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using Microsoft.UI.Xaml;

namespace TableViewSampleApp.Services;

/// <summary>
/// Tiny key/value persistence for the unpackaged sample. We do NOT use
/// Windows.Storage.ApplicationData here because that surface throws on
/// unpackaged WinUI 3 apps (no app identity). A single-line text file
/// in %LocalAppData%\TableViewSampleApp\theme.txt is enough for the one
/// setting we expose (Default | Light | Dark).
/// </summary>
internal static class AppSettings
{
    private const string ThemeFileName = "theme.txt";

    /// <summary>
    /// Event raised whenever the persisted theme changes via
    /// <see cref="SaveTheme"/>. Listeners (the shell button glyph,
    /// the Settings page radios when visible) re-sync from here.
    /// </summary>
    public static event EventHandler<ElementTheme>? ThemeChanged;

    public static ElementTheme LoadTheme()
    {
        try
        {
            var path = GetThemeFilePath();
            if (!File.Exists(path)) return ElementTheme.Default;

            var text = File.ReadAllText(path).Trim();
            return text switch
            {
                "Light" => ElementTheme.Light,
                "Dark" => ElementTheme.Dark,
                _ => ElementTheme.Default,
            };
        }
        catch
        {
            // Storage failures are non-fatal — the sample just falls back
            // to the system theme. Settings page reflects that on reload.
            return ElementTheme.Default;
        }
    }

    public static void SaveTheme(ElementTheme theme)
    {
        try
        {
            var path = GetThemeFilePath();
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.WriteAllText(path, theme.ToString());
        }
        catch
        {
            // Silently ignore — the in-memory theme is still applied to
            // the running window via RequestedTheme.
        }

        ThemeChanged?.Invoke(null, theme);
    }

    /// <summary>
    /// Apply a theme to the live window root and persist it. Single entry
    /// point used by both the shell toggle button and the Settings page.
    /// </summary>
    public static void ApplyAndPersist(FrameworkElement? root, ElementTheme theme)
    {
        if (root is not null)
        {
            root.RequestedTheme = theme;
        }

        SaveTheme(theme);
    }

    private static string GetThemeFilePath()
    {
        var baseDir = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        return Path.Combine(baseDir, "TableViewSampleApp", ThemeFileName);
    }
}
